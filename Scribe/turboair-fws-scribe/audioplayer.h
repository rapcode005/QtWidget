#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QtWidgets>

class AudioPlayer : public QObject
{
Q_OBJECT
public:
    AudioPlayer(QPushButton *button, QComboBox *wavFileListComboBox);
    QPushButton *playPause;
    QMediaPlaylist *fileList;
    QMediaPlayer *player;
    QComboBox *wavFileList;
    int status;

public slots:
    void currentIndexChanged(int position);

private slots:
    void durationChanged(quint64 progress);
    void positionChanged(quint64 progress);
    void stateChanged(QMediaPlayer::State state);

signals:
    void sendCurrentState(const int &status);

};

#endif // AUDIOPLAYER_H
