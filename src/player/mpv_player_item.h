// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_PLAYER_MPV_PLAYER_ITEM_H
#define LOCALPLEX_PLAYER_MPV_PLAYER_ITEM_H

#include <QtQuick/QQuickFramebufferObject>
#include <QVariantList>

struct mpv_handle;
struct mpv_render_context;

namespace localplex::player {

class MpvRenderer;

class MpvPlayerItem : public QQuickFramebufferObject {
    Q_OBJECT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaTitleChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged)
    Q_PROPERTY(bool coreIdle READ isCoreIdle NOTIFY coreIdleChanged)

public:
    explicit MpvPlayerItem(QQuickItem *parent = nullptr);
    ~MpvPlayerItem() override;

    Renderer *createRenderer() const override;

    QString source() const;
    void setSource(const QString &source);

    double position() const;
    double duration() const;

    bool isPaused() const;
    void setPaused(bool paused);

    double volume() const;
    void setVolume(double volume);

    bool isMuted() const;
    void setMuted(bool muted);

    double speed() const;
    void setSpeed(double speed);

    QString mediaTitle() const;
    bool hasMedia() const;
    bool isCoreIdle() const;

    Q_INVOKABLE void loadFile(const QString &filePath);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(double seconds, bool relative = false);
    Q_INVOKABLE void command(const QVariantList &args);

signals:
    void sourceChanged(const QString &source);
    void positionChanged(double position);
    void durationChanged(double duration);
    void pausedChanged(bool paused);
    void volumeChanged(double volume);
    void mutedChanged(bool muted);
    void speedChanged(double speed);
    void mediaTitleChanged(const QString &title);
    void hasMediaChanged(bool hasMedia);
    void coreIdleChanged(bool idle);
    void playbackEnded();
    void fileLoaded();
    void requestUpdate();

private slots:
    void processMpvEvents();
    void onMpvRedrawRequested();

private:
    static void onMpvWakeup(void *ctx);
    static void onMpvRedraw(void *ctx);

    void setPropertyInternal(const char *name, double value);
    void setPropertyInternal(const char *name, bool value);
    void setPropertyInternal(const char *name, const char *value);

    mpv_handle *m_mpvHandle{nullptr};
    mutable mpv_render_context *m_mpvRenderContext{nullptr};

    QString m_source;
    double m_position{0.0};
    double m_duration{0.0};
    bool m_paused{true};
    double m_volume{100.0};
    bool m_muted{false};
    double m_speed{1.0};
    QString m_mediaTitle;
    bool m_hasMedia{false};
    bool m_coreIdle{true};

    friend class MpvRenderer;
};

} // namespace localplex::player

#endif // LOCALPLEX_PLAYER_MPV_PLAYER_ITEM_H
