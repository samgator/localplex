// SPDX-License-Identifier: GPL-3.0-or-later

#include "mpv_player_item.h"
#include "mpv_node_converter.h"
#include "../database/playback_database.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickOpenGLUtils>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QMetaObject>
#include <QtCore/QDebug>
#include <cstring>
#include <algorithm>
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
    mpv_observe_property(m_mpvHandle, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpvHandle, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpvHandle, 0, "chapter", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpvHandle, 0, "demuxer-cache-state", MPV_FORMAT_NODE);

    mpv_set_wakeup_callback(m_mpvHandle, onMpvWakeup, this);

    connect(this, &MpvPlayerItem::requestUpdate, this, &MpvPlayerItem::onMpvRedrawRequested, Qt::QueuedConnection);

    // Save playback position periodically every 5 seconds while playing
    m_saveTimer.setInterval(5000);
    connect(&m_saveTimer, &QTimer::timeout, this, &MpvPlayerItem::saveCurrentProgress);
}

MpvPlayerItem::~MpvPlayerItem() {
    saveCurrentProgress();
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

double MpvPlayerItem::bufferPosition() const {
    return m_bufferPosition;
}

bool MpvPlayerItem::isPaused() const {
    return m_paused;
}

void MpvPlayerItem::setPaused(bool paused) {
    if (m_paused == paused) {
        return;
    }
    setPropertyInternal("pause", paused);
    if (paused) {
        saveCurrentProgress();
        m_saveTimer.stop();
    } else {
        m_saveTimer.start();
    }
}

double MpvPlayerItem::volume() const {
    return m_volume;
}

void MpvPlayerItem::setVolume(double volume) {
    volume = std::clamp(volume, 0.0, 100.0);
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
    speed = std::clamp(speed, 0.25, 4.0);
    if (m_speed == speed) {
        return;
    }
    setPropertyInternal("speed", speed);
}

void MpvPlayerItem::adjustSpeed(double delta) {
    setSpeed(m_speed + delta);
}

void MpvPlayerItem::resetSpeed() {
    setSpeed(1.0);
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

QVariantList MpvPlayerItem::audioTracks() const {
    return m_audioTracks;
}

QVariantList MpvPlayerItem::subtitleTracks() const {
    return m_subtitleTracks;
}

int MpvPlayerItem::currentAudioTrackId() const {
    return m_currentAudioTrackId;
}

void MpvPlayerItem::setAudioTrackId(int id) {
    setAudioTrack(id);
}

int MpvPlayerItem::currentSubtitleTrackId() const {
    return m_currentSubtitleTrackId;
}

void MpvPlayerItem::setSubtitleTrackId(int id) {
    setSubtitleTrack(id);
}

QString MpvPlayerItem::currentAudioLanguage() const {
    return m_currentAudioLang;
}

QString MpvPlayerItem::currentSubtitleLanguage() const {
    return m_currentSubtitleLang;
}

QVariantList MpvPlayerItem::chapters() const {
    return m_chapters;
}

int MpvPlayerItem::currentChapter() const {
    return m_currentChapter;
}

void MpvPlayerItem::setAudioTrack(int trackId) {
    if (!m_mpvHandle) {
        return;
    }
    if (trackId < 0) {
        const char *cmd[] = {"set", "aid", "no", nullptr};
        mpv_command_async(m_mpvHandle, 0, cmd);
    } else {
        QByteArray trackStr = QByteArray::number(trackId);
        const char *cmd[] = {"set", "aid", trackStr.constData(), nullptr};
        mpv_command_async(m_mpvHandle, 0, cmd);
    }

    // Remember preferred audio language for the show
    if (!m_currentAudioLang.isEmpty()) {
        const QString showKey = extractShowKey(m_source);
        database::PlaybackDatabase::instance().saveShowPreferences(showKey, m_currentAudioLang, QString());
    }
}

void MpvPlayerItem::setSubtitleTrack(int trackId) {
    if (!m_mpvHandle) {
        return;
    }
    if (trackId <= 0) {
        const char *cmd[] = {"set", "sid", "no", nullptr};
        mpv_command_async(m_mpvHandle, 0, cmd);
        m_currentSubtitleTrackId = -1;
        emit currentSubtitleTrackIdChanged(-1);
    } else {
        QByteArray trackStr = QByteArray::number(trackId);
        const char *cmd[] = {"set", "sid", trackStr.constData(), nullptr};
        mpv_command_async(m_mpvHandle, 0, cmd);
    }

    // Remember preferred subtitle language for the show
    if (!m_currentSubtitleLang.isEmpty()) {
        const QString showKey = extractShowKey(m_source);
        database::PlaybackDatabase::instance().saveShowPreferences(showKey, QString(), m_currentSubtitleLang);
    }
}

void MpvPlayerItem::cycleAudioTrack() {
    if (!m_mpvHandle) {
        return;
    }
    const char *cmd[] = {"cycle", "aid", nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
}

void MpvPlayerItem::cycleSubtitleTrack() {
    if (!m_mpvHandle) {
        return;
    }
    const char *cmd[] = {"cycle", "sid", nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
}

void MpvPlayerItem::nextChapter() {
    if (!m_mpvHandle) {
        return;
    }
    const char *cmd[] = {"add", "chapter", "1", nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
}

void MpvPlayerItem::previousChapter() {
    if (!m_mpvHandle) {
        return;
    }
    const char *cmd[] = {"add", "chapter", "-1", nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
}

void MpvPlayerItem::markWatched(bool watched) {
    if (m_source.isEmpty()) {
        return;
    }
    database::PlaybackDatabase::instance().markWatched(m_source, watched);
}

void MpvPlayerItem::loadFile(const QString &filePath) {
    if (!m_mpvHandle || filePath.isEmpty()) {
        return;
    }

    // Save previous file's progress before switching
    saveCurrentProgress();

    QString cleanPath = filePath;
    if (cleanPath.startsWith(QLatin1String("file://"))) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }

    if (!QFile::exists(cleanPath)) {
        emit playbackError(QStringLiteral("The file at \"%1\" could not be located. It may have been moved or the drive disconnected.").arg(cleanPath));
        return;
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
    saveCurrentProgress();
    if (!m_mpvHandle) {
        return;
    }
    const char *cmd[] = {"stop", nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
    m_hasMedia = false;
    m_saveTimer.stop();
    emit hasMediaChanged(false);
}

void MpvPlayerItem::seek(double seconds, bool relative) {
    if (!m_mpvHandle) {
        return;
    }
    QByteArray secStr = QByteArray::number(seconds);
    const char *mode = relative ? "relative+exact" : "absolute+exact";
    const char *cmd[] = {"seek", secStr.constData(), mode, nullptr};
    mpv_command_async(m_mpvHandle, 0, cmd);
    saveCurrentProgress();
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

void MpvPlayerItem::saveCurrentProgress() {
    if (m_source.isEmpty() || m_position <= 0.0 || m_duration <= 0.0) {
        return;
    }

    database::PlaybackDatabase::instance().saveProgress(
        m_source,
        m_position,
        m_duration,
        m_currentAudioTrackId,
        m_currentSubtitleTrackId,
        m_currentAudioLang,
        m_currentSubtitleLang
    );
    m_lastSavedPosition = m_position;
}

QString MpvPlayerItem::extractShowKey(const QString &filePath) const {
    QFileInfo fi(filePath);
    QDir parentDir = fi.dir();
    QString dirName = parentDir.dirName();

    // Check if immediate folder is a season folder, e.g. "Season 1"
    if (dirName.startsWith(QLatin1String("Season"), Qt::CaseInsensitive) ||
        dirName.startsWith(QLatin1String("Series"), Qt::CaseInsensitive) ||
        dirName.startsWith(QLatin1String("Specials"), Qt::CaseInsensitive)) {
        parentDir.cdUp();
        return parentDir.dirName();
    }
    return dirName;
}

void MpvPlayerItem::parseTrackList(const QVariantList &tracks) {
    QVariantList newAudio;
    QVariantList newSubs;
    int activeAudio = -1;
    int activeSub = -1;
    QString activeAudioLang;
    QString activeSubLang;

    for (const auto &trackVal : tracks) {
        QVariantMap track = trackVal.toMap();
        QString type = track.value(QStringLiteral("type")).toString();
        int id = track.value(QStringLiteral("id")).toInt();
        QString title = track.value(QStringLiteral("title")).toString();
        QString lang = track.value(QStringLiteral("lang")).toString();
        bool selected = track.value(QStringLiteral("selected")).toBool();

        QString displayTitle = title.isEmpty() ? (lang.isEmpty() ? QStringLiteral("Track %1").arg(id) : lang.toUpper()) : title;

        QVariantMap entry;
        entry[QStringLiteral("id")] = id;
        entry[QStringLiteral("title")] = displayTitle;
        entry[QStringLiteral("lang")] = lang;
        entry[QStringLiteral("selected")] = selected;

        if (type == QLatin1String("audio")) {
            newAudio.append(entry);
            if (selected) {
                activeAudio = id;
                activeAudioLang = lang;
            }
        } else if (type == QLatin1String("sub")) {
            newSubs.append(entry);
            if (selected) {
                activeSub = id;
                activeSubLang = lang;
            }
        }
    }

    m_audioTracks = newAudio;
    m_subtitleTracks = newSubs;
    m_currentAudioTrackId = activeAudio;
    m_currentSubtitleTrackId = activeSub;
    m_currentAudioLang = activeAudioLang;
    m_currentSubtitleLang = activeSubLang;

    emit audioTracksChanged();
    emit subtitleTracksChanged();
    emit currentAudioTrackIdChanged(activeAudio);
    emit currentSubtitleTrackIdChanged(activeSub);
    emit currentAudioLanguageChanged(activeAudioLang);
    emit currentSubtitleLanguageChanged(activeSubLang);
}

void MpvPlayerItem::parseChapterList(const QVariantList &chapterList) {
    QVariantList list;
    for (const auto &cVal : chapterList) {
        QVariantMap cMap = cVal.toMap();
        QVariantMap entry;
        entry[QStringLiteral("title")] = cMap.value(QStringLiteral("title")).toString();
        entry[QStringLiteral("time")] = cMap.value(QStringLiteral("time")).toDouble();
        list.append(entry);
    }
    m_chapters = list;
    emit chaptersChanged();
}

void MpvPlayerItem::parseDemuxerCache(const QVariantMap &cacheMap) {
    if (m_duration <= 0.0) {
        return;
    }
    // "reader-pts" or "cache-end" indicates how far ahead video is buffered
    double cacheEnd = cacheMap.value(QStringLiteral("cache-end")).toDouble();
    if (cacheEnd > 0.0) {
        m_bufferPosition = cacheEnd;
        emit bufferPositionChanged(m_bufferPosition);
    }
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
            } else if (std::strcmp(prop->name, "chapter") == 0) {
                if (prop->format == MPV_FORMAT_INT64) {
                    int val = static_cast<int>(*static_cast<int64_t *>(prop->data));
                    if (val != m_currentChapter) {
                        m_currentChapter = val;
                        emit currentChapterChanged(m_currentChapter);
                    }
                }
            } else if (std::strcmp(prop->name, "track-list") == 0) {
                if (prop->format == MPV_FORMAT_NODE) {
                    QVariant v = mpvNodeToVariant(static_cast<mpv_node *>(prop->data));
                    parseTrackList(v.toList());
                }
            } else if (std::strcmp(prop->name, "chapter-list") == 0) {
                if (prop->format == MPV_FORMAT_NODE) {
                    QVariant v = mpvNodeToVariant(static_cast<mpv_node *>(prop->data));
                    parseChapterList(v.toList());
                }
            } else if (std::strcmp(prop->name, "demuxer-cache-state") == 0) {
                if (prop->format == MPV_FORMAT_NODE) {
                    QVariant v = mpvNodeToVariant(static_cast<mpv_node *>(prop->data));
                    parseDemuxerCache(v.toMap());
                }
            }
            break;
        }
        case MPV_EVENT_FILE_LOADED: {
            m_hasMedia = true;
            m_saveTimer.start();
            emit hasMediaChanged(true);
            emit fileLoaded();

            // Check saved progress from local database for auto-resume
            if (!m_source.isEmpty()) {
                QVariantMap progress = database::PlaybackDatabase::instance().getProgress(m_source);
                if (!progress.isEmpty()) {
                    double savedPos = progress.value(QStringLiteral("position")).toDouble();
                    bool completed = progress.value(QStringLiteral("completed")).toBool();

                    // If file was previously watched (> 5s) and not completed, resume with 2.5s context rewind
                    if (savedPos > 5.0 && !completed) {
                        double resumePos = std::max(0.0, savedPos - 2.5);
                        seek(resumePos, false);
                        emit resumed(resumePos, savedPos);
                    }
                }

                // Apply per-show language preferences if present
                const QString showKey = extractShowKey(m_source);
                QVariantMap showPrefs = database::PlaybackDatabase::instance().getShowPreferences(showKey);
                QString prefAudio = showPrefs.value(QStringLiteral("audioLang")).toString();
                QString prefSub = showPrefs.value(QStringLiteral("subtitleLang")).toString();

                if (!prefAudio.isEmpty()) {
                    QByteArray prefAudioUtf8 = prefAudio.toUtf8();
                    const char *cmd[] = {"set", "alang", prefAudioUtf8.constData(), nullptr};
                    mpv_command_async(m_mpvHandle, 0, cmd);
                }
                if (!prefSub.isEmpty()) {
                    QByteArray prefSubUtf8 = prefSub.toUtf8();
                    const char *cmd[] = {"set", "slang", prefSubUtf8.constData(), nullptr};
                    mpv_command_async(m_mpvHandle, 0, cmd);
                }
            }
            break;
        }
        case MPV_EVENT_END_FILE: {
            auto *endFile = static_cast<mpv_event_end_file *>(event->data);
            if (endFile && endFile->reason == MPV_END_FILE_REASON_ERROR) {
                emit playbackError(QStringLiteral("The file could not be decoded. The container or video stream may be corrupt."));
            } else {
                saveCurrentProgress();
                emit playbackEnded();
            }
            break;
        }
        default:
            break;
        }
    }
}

} // namespace localplex::player
