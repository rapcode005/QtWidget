#include "audioplayer.h"
#include <QMediaPlayer>

AudioPlayer::AudioPlayer(QPushButton *button, QComboBox *wavFileListComboBox)
{
    player = new QMediaPlayer();
    player->setNotifyInterval(1000);
    fileList = new QMediaPlaylist(this);

    fileList->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);

    playPause = button;
    wavFileList = wavFileListComboBox;

    //connecting all the signals from our mediaplayer object to functions in this class
    connect(player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(player, &QMediaPlayer::stateChanged, this, &AudioPlayer::stateChanged);
    connect(fileList, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
}


/**
 * Converts time in milliseconds to MM:SS
 * @brief formatTime
 * @param timeMilliSeconds
 * @return QStringLiteral

static QString formatTime(qint64 timeMilliSeconds)
{
    qint64 seconds = timeMilliSeconds / 1000;
    const qint64 minutes = seconds / 60;
    seconds -= minutes * 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
*/

void AudioPlayer::durationChanged(quint64 duration){

    Q_UNUSED(duration)
    //   durText->setText("/" + formatTime(duration));
    //--    curFileText->setText(fileList->currentMedia().canonicalUrl().fileName());
    //int f = fileList->currentIndex() + 1;
    //int l = fileList->mediaCount();
    //--   curIndexText->setText("(" + QString::number(f) + "/" + QString::number(l) + ")");
}


void AudioPlayer::positionChanged(quint64 progress){

    Q_UNUSED(progress)
    if (player->position() == player->duration()){
        if (fileList->currentIndex() + 1 == fileList->mediaCount()){
            player->pause();
            player->setPosition(0);
        }
    }
}


void AudioPlayer::stateChanged(QMediaPlayer::State state){
    //handling the states of our media player, currently we only set our buttons text, but it's expandable
    if (state == QMediaPlayer::StoppedState){
       //playPause->setText(">");
        //isPlay = false;
        status = 2;
    }
    else if (state == QMediaPlayer::PlayingState){
       //playPause->setText("||");
        status = 1;
    }
    else if (state == QMediaPlayer::PausedState){
        //playPause->setText(">");
        status = 0;
    }
    emit sendCurrentState(status);
}

void AudioPlayer::currentIndexChanged(int position){
    //qDebug() << "Setting position to: " << position;
    if(position != -1){
        wavFileList->setCurrentText(wavFileList->itemText(position));
    }
}

