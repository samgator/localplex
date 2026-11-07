// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_PLAYER_MPV_PLAYER_ITEM_H
#define LOCALPLEX_PLAYER_MPV_PLAYER_ITEM_H

#include <QtQuick/QQuickFramebufferObject>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/QTimer>

struct mpv_handle;
struct mpv_render_context;

namespace localplex::player {

class MpvRenderer;

class MpvPlayerItem : public QQuickFramebufferObject {
    Q_OBJECT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double bufferPosition READ bufferPosition NOTIFY bufferPositionChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaTitleChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged)
    Q_PROPERTY(bool coreIdle READ isCoreIdle NOTIFY coreIdleChanged)

    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY audioTracksChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY subtitleTracksChanged)
    Q_PROPERTY(int currentAudioTrackId READ currentAudioTrackId WRITE setAudioTrackId NOTIFY currentAudioTrackIdChanged)
    Q_PROPERTY(int currentSubtitleTrackId READ currentSubtitleTrackId WRITE setSubtitleTrackId NOTIFY currentSubtitleTrackIdChanged)
    Q_PROPERTY(QString currentAudioLanguage READ currentAudioLanguage NOTIFY currentAudioLanguageChanged)
    Q_PROPERTY(QString currentSubtitleLanguage READ currentSubtitleLanguage NOTIFY currentSubtitleLanguageChanged)

    Q_PROPERTY(QVariantList chapters READ chapters NOTIFY chaptersChanged)
    Q_PROPERTY(int currentChapter READ currentChapter NOTIFY currentChapterChanged)

public:
    explicit MpvPlayerItem(QQuickItem *parent = nullptr);
    ~MpvPlayerItem() override;

    Renderer *createRenderer() const override;

    QString source() const;
    void setSource(const QString &source);

    double position() const;
    double duration() const;
    double bufferPosition() const;

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

    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;
    int currentAudioTrackId() const;
    void setAudioTrackId(int id);
    int currentSubtitleTrackId() const;
    void setSubtitleTrackId(int id);
    QString currentAudioLanguage() const;
    QString currentSubtitleLanguage() const;

    QVariantList chapters() const;
    int currentChapter() const;

    Q_INVOKABLE void loadFile(const QString &filePath);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(double seconds, bool relative = false);
    Q_INVOKABLE void adjustSpeed(double delta);
    Q_INVOKABLE void resetSpeed();

    Q_INVOKABLE void setAudioTrack(int trackId);
    Q_INVOKABLE void setSubtitleTrack(int trackId);
    Q_INVOKABLE void cycleAudioTrack();
    Q_INVOKABLE void cycleSubtitleTrack();

    Q_INVOKABLE void nextChapter();
    Q_INVOKABLE void previousChapter();
    Q_INVOKABLE void markWatched(bool watched);

    Q_INVOKABLE void command(const QVariantList &args);

signals:
    void sourceChanged(const QString &source);
    void positionChanged(double position);
    void durationChanged(double duration);
    void bufferPositionChanged(double bufferPosition);
    void pausedChanged(bool paused);
    void volumeChanged(double volume);
    void mutedChanged(bool muted);
    void speedChanged(double speed);
    void mediaTitleChanged(const QString &title);
    void hasMediaChanged(bool hasMedia);
    void coreIdleChanged(bool idle);

    void audioTracksChanged();
    void subtitleTracksChanged();
    void currentAudioTrackIdChanged(int id);
    void currentSubtitleTrackIdChanged(int id);
    void currentAudioLanguageChanged(const QString &lang);
    void currentSubtitleLanguageChanged(const QString &lang);

    void chaptersChanged();
    void currentChapterChanged(int chapter);

    void resumed(double resumePosition, double originalPosition);
    void playbackEnded();
    void playbackError(const QString &message);
    void fileLoaded();
    void requestUpdate();

private slots:
    void processMpvEvents();
    void onMpvRedrawRequested();
    void saveCurrentProgress();

private:
    static void onMpvWakeup(void *ctx);
    static void onMpvRedraw(void *ctx);

    void parseTrackList(const QVariantList &tracks);
    void parseChapterList(const QVariantList &chapters);
    void parseDemuxerCache(const QVariantMap &cacheMap);

    void setPropertyInternal(const char *name, double value);
    void setPropertyInternal(const char *name, bool value);
    void setPropertyInternal(const char *name, const char *value);

    QString extractShowKey(const QString &filePath) const;

    mpv_handle *m_mpvHandle{nullptr};
    mutable mpv_render_context *m_mpvRenderContext{nullptr};

    QString m_source;
    double m_position{0.0};
    double m_duration{0.0};
    double m_bufferPosition{0.0};
    bool m_paused{true};
    double m_volume{100.0};
    bool m_muted{false};
    double m_speed{1.0};
    QString m_mediaTitle;
    bool m_hasMedia{false};
    bool m_coreIdle{true};

    QVariantList m_audioTracks;
    QVariantList m_subtitleTracks;
    int m_currentAudioTrackId{-1};
    int m_currentSubtitleTrackId{-1};
    QString m_currentAudioLang;
    QString m_currentSubtitleLang;

    QVariantList m_chapters;
    int m_currentChapter{0};

    QTimer m_saveTimer;
    double m_lastSavedPosition{0.0};

    friend class MpvRenderer;
};

} // namespace localplex::player

#endif // LOCALPLEX_PLAYER_MPV_PLAYER_ITEM_H
