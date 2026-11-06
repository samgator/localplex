// SPDX-License-Identifier: GPL-3.0-or-later

#include "mpv_player_item.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickOpenGLUtils>
#include <QtCore/QUrl>
#include <QtCore/QMetaObject>
#include <QtCore/QDebug>
#include <cstring>
#include <vector>

namespace localplex::player {

namespace {

void *getProcAddress(void *ctx, const char *name) {
    Q_UNUSED(ctx)
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx) {
        return nullptr;
    }
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}

} // namespace

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit MpvRenderer(MpvPlayerItem *item)
        : m_item(item) {}

    ~MpvRenderer() override {
        if (m_item && m_item->m_mpvRenderContext) {
            mpv_render_context_free(m_item->m_mpvRenderContext);
            m_item->m_mpvRenderContext = nullptr;
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        if (!m_item->m_mpvRenderContext && m_item->m_mpvHandle) {
            mpv_opengl_init_params glInitParams{getProcAddress, nullptr};
            mpv_render_param params[] = {
                {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInitParams},
                {MPV_RENDER_PARAM_INVALID, nullptr}
            };

            int err = mpv_render_context_create(&m_item->m_mpvRenderContext, m_item->m_mpvHandle, params);
            if (err < 0) {
                qWarning() << "Failed to initialize mpv OpenGL render context:" << mpv_error_string(err);
            } else {
                mpv_render_context_set_update_callback(m_item->m_mpvRenderContext, MpvPlayerItem::onMpvRedraw, m_item);
            }
        }
        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }

    void render() override {
        if (!m_item || !m_item->m_mpvRenderContext) {
            return;
        }

        QOpenGLFramebufferObject *fbo = framebufferObject();
        if (!fbo) {
            return;
        }

        m_item->window()->beginExternalCommands();
        QQuickOpenGLUtils::resetOpenGLState();

        mpv_opengl_fbo mpfbo{
            .fbo = static_cast<int>(fbo->handle()),
            .w = fbo->width(),
            .h = fbo->height(),
            .internal_format = 0
        };

        int flipY = 0;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flipY},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        mpv_render_context_render(m_item->m_mpvRenderContext, params);

        QQuickOpenGLUtils::resetOpenGLState();
        m_item->window()->endExternalCommands();
    }

private:
    MpvPlayerItem *m_item{nullptr};
};

MpvPlayerItem::MpvPlayerItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent) {
    m_mpvHandle = mpv_create();
    if (!m_mpvHandle) {
        qWarning() << "Failed to allocate mpv handle";
        return;
    }

    mpv_set_option_string(m_mpvHandle, "vo", "libmpv");
    mpv_set_option_string(m_mpvHandle, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpvHandle, "terminal", "no");
    mpv_set_option_string(m_mpvHandle, "msg-level", "all=warn");
    mpv_set_option_string(m_mpvHandle, "keep-open", "yes");

    if (mpv_initialize(m_mpvHandle) < 0) {
        qWarning() << "Failed to initialize mpv handle";
        mpv_destroy(m_mpvHandle);
        m_mpvHandle = nullptr;
        return;
    }

    mpv_observe_property(m_mpvHandle, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpvHandle, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpvHandle, 0, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "media-title", MPV_FORMAT_STRING);
    mpv_observe_property(m_mpvHandle, 0, "core-idle", MPV_FORMAT_FLAG);

    mpv_set_wakeup_callback(m_mpvHandle, onMpvWakeup, this);

    connect(this, &MpvPlayerItem::requestUpdate, this, &MpvPlayerItem::onMpvRedrawRequested, Qt::QueuedConnection);
}

MpvPlayerItem::~MpvPlayerItem() {
    if (m_mpvHandle) {
        mpv_set_wakeup_callback(m_mpvHandle, nullptr, nullptr);
        if (m_mpvRenderContext) {
            mpv_render_context_free(m_mpvRenderContext);
            m_mpvRenderContext = nullptr;
        }
        mpv_terminate_destroy(m_mpvHandle);
        m_mpvHandle = nullptr;
    }
}

QQuickFramebufferObject::Renderer *MpvPlayerItem::createRenderer() const {
    if (window()) {
        window()->setPersistentGraphics(true);
        window()->setPersistentSceneGraph(true);
    }
    return new MpvRenderer(const_cast<MpvPlayerItem *>(this));
}

QString MpvPlayerItem::source() const {
    return m_source;
}

void MpvPlayerItem::setSource(const QString &source) {
    if (m_source == source) {
        return;
    }
    loadFile(source);
}

double MpvPlayerItem::position() const {
    return m_position;
}

double MpvPlayerItem::duration() const {
    return m_duration;
}

bool MpvPlayerItem::isPaused() const {
    return m_paused;
}

void MpvPlayerItem::setPaused(bool paused) {
    if (m_paused == paused) {
        return;
    }
    setPropertyInternal("pause", paused);
}

double MpvPlayerItem::volume() const {
    return m_volume;
}

void MpvPlayerItem::setVolume(double volume) {
    if (m_volume == volume) {
        return;
    }
    setPropertyInternal("volume", volume);
}

bool MpvPlayerItem::isMuted() const {
    return m_muted;
}

void MpvPlayerItem::setMuted(bool muted) {
    if (m_muted == muted) {
        return;
    }
    setPropertyInternal("mute", muted);
}

double MpvPlayerItem::speed() const {
    return m_speed;
}

void MpvPlayerItem::setSpeed(double speed) {
    if (m_speed == speed) {
        return;
    }
    setPropertyInternal("speed", speed);
}

QString MpvPlayerItem::mediaTitle() const {
    return m_mediaTitle;
}

bool MpvPlayerItem::hasMedia() const {
    return m_hasMedia;
}

bool MpvPlayerItem::isCoreIdle() const {
    return m_coreIdle;
}

void MpvPlayerItem::loadFile(const QString &filePath) {
    if (!m_mpvHandle || filePath.isEmpty()) {
        return;
    }

    QString cleanPath = filePath;
    if (cleanPath.startsWith(QLatin1String("file://"))) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }

    m_source = cleanPath;
    emit sourceChanged(m_source);

    QByteArray pathBytes = cleanPath.toUtf8();
    const char *cmd[] = {"loadfile", pathBytes.constData(), nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
}

void MpvPlayerItem::play() {
    setPaused(false);
}

void MpvPlayerItem::pause() {
    setPaused(true);
}

void MpvPlayerItem::togglePause() {
    setPaused(!m_paused);
}

void MpvPlayerItem::stop() {
    if (!m_mpvHandle) {
        return;
    }
    const char *cmd[] = {"stop", nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
    m_hasMedia = false;
    emit hasMediaChanged(false);
}

void MpvPlayerItem::seek(double seconds, bool relative) {
    if (!m_mpvHandle) {
        return;
    }
    QByteArray secStr = QByteArray::number(seconds);
    const char *mode = relative ? "relative" : "absolute";
    const char *cmd[] = {"seek", secStr.constData(), mode, nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
}

void MpvPlayerItem::command(const QVariantList &args) {
    if (!m_mpvHandle || args.isEmpty()) {
        return;
    }

    std::vector<QByteArray> storage;
    storage.reserve(args.size());
    std::vector<const char *> cargs;
    cargs.reserve(args.size() + 1);

    for (const auto &arg : args) {
        storage.push_back(arg.toString().toUtf8());
        cargs.push_back(storage.back().constData());
    }
    cargs.push_back(nullptr);

    mpv_command_async(m_mpvHandle, 0, cargs.data());
}

void MpvPlayerItem::setPropertyInternal(const char *name, double value) {
    if (!m_mpvHandle) {
        return;
    }
    mpv_set_property_async(m_mpvHandle, 0, name, MPV_FORMAT_DOUBLE, &value);
}

void MpvPlayerItem::setPropertyInternal(const char *name, bool value) {
    if (!m_mpvHandle) {
        return;
    }
    int flag = value ? 1 : 0;
    mpv_set_property_async(m_mpvHandle, 0, name, MPV_FORMAT_FLAG, &flag);
}

void MpvPlayerItem::setPropertyInternal(const char *name, const char *value) {
    if (!m_mpvHandle) {
        return;
    }
    mpv_set_property_async(m_mpvHandle, 0, name, MPV_FORMAT_STRING, &value);
}

void MpvPlayerItem::onMpvWakeup(void *ctx) {
    auto *self = static_cast<MpvPlayerItem *>(ctx);
    if (self) {
        QMetaObject::invokeMethod(self, &MpvPlayerItem::processMpvEvents, Qt::QueuedConnection);
    }
}

void MpvPlayerItem::onMpvRedraw(void *ctx) {
    auto *self = static_cast<MpvPlayerItem *>(ctx);
    if (self) {
        emit self->requestUpdate();
    }
}

void MpvPlayerItem::onMpvRedrawRequested() {
    update();
}

void MpvPlayerItem::processMpvEvents() {
    if (!m_mpvHandle) {
        return;
    }

    while (true) {
        mpv_event *event = mpv_wait_event(m_mpvHandle, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }

        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *prop = static_cast<mpv_event_property *>(event->data);
            if (!prop || !prop->name) {
                break;
            }

            if (std::strcmp(prop->name, "time-pos") == 0) {
                if (prop->format == MPV_FORMAT_DOUBLE) {
                    double val = *static_cast<double *>(prop->data);
                    if (val != m_position) {
                        m_position = val;
                        emit positionChanged(m_position);
                    }
                }
            } else if (std::strcmp(prop->name, "duration") == 0) {
                if (prop->format == MPV_FORMAT_DOUBLE) {
                    double val = *static_cast<double *>(prop->data);
                    if (val != m_duration) {
                        m_duration = val;
                        emit durationChanged(m_duration);
                    }
                }
            } else if (std::strcmp(prop->name, "pause") == 0) {
                if (prop->format == MPV_FORMAT_FLAG) {
                    bool val = (*static_cast<int *>(prop->data) != 0);
                    if (val != m_paused) {
                        m_paused = val;
                        emit pausedChanged(m_paused);
                    }
                }
            } else if (std::strcmp(prop->name, "volume") == 0) {
                if (prop->format == MPV_FORMAT_DOUBLE) {
                    double val = *static_cast<double *>(prop->data);
                    if (val != m_volume) {
                        m_volume = val;
                        emit volumeChanged(m_volume);
                    }
                }
            } else if (std::strcmp(prop->name, "mute") == 0) {
                if (prop->format == MPV_FORMAT_FLAG) {
                    bool val = (*static_cast<int *>(prop->data) != 0);
                    if (val != m_muted) {
                        m_muted = val;
                        emit mutedChanged(m_muted);
                    }
                }
            } else if (std::strcmp(prop->name, "speed") == 0) {
                if (prop->format == MPV_FORMAT_DOUBLE) {
                    double val = *static_cast<double *>(prop->data);
                    if (val != m_speed) {
                        m_speed = val;
                        emit speedChanged(m_speed);
                    }
                }
            } else if (std::strcmp(prop->name, "media-title") == 0) {
                if (prop->format == MPV_FORMAT_STRING) {
                    const char *val = *static_cast<char **>(prop->data);
                    QString title = val ? QString::fromUtf8(val) : QString();
                    if (title != m_mediaTitle) {
                        m_mediaTitle = title;
                        emit mediaTitleChanged(m_mediaTitle);
                    }
                }
            } else if (std::strcmp(prop->name, "core-idle") == 0) {
                if (prop->format == MPV_FORMAT_FLAG) {
                    bool val = (*static_cast<int *>(prop->data) != 0);
                    if (val != m_coreIdle) {
                        m_coreIdle = val;
                        emit coreIdleChanged(m_coreIdle);
                    }
                }
            }
            break;
        }
        case MPV_EVENT_FILE_LOADED: {
            m_hasMedia = true;
            emit hasMediaChanged(true);
            emit fileLoaded();
            break;
        }
        case MPV_EVENT_END_FILE: {
            emit playbackEnded();
            break;
        }
        default:
            break;
        }
    }
}

} // namespace localplex::player
