#include "precompiled.h"

#include "RecitationHelper.h"
#include "AppLogFetcher.h"
#include "CommonConstants.h"
#include "InvocationUtils.h"
#include "IOUtils.h"
#include "Logger.h"
#include "Persistance.h"
#include "TextUtils.h"

#define normalize(a) TextUtils::zeroFill(a,3)
#define PLAYLIST_TARGET QString("%1/playlist.m3u").arg( QDir::tempPath() )
#define ITERATION 20
#define CHUNK_SIZE 4
#define COOKIE_RECITATION_MP3 "recitation"
#define ANCHOR_KEY "anchor"
#define PLAYLIST_KEY "playlist"
#define LOCAL_PATH "local"
#define KEY_CHAPTER "chapter"
#define KEY_TRANSFER_NAME "name"
#define KEY_VERSE "verse"
#define KEY_QUEUE "queue"
#define PLAYLIST_ERROR "error"

using namespace canadainc;

namespace {

QVariantMap writeVerse(QVariant const& cookie, QByteArray const& data)
{
    QVariantMap q = cookie.toMap();

    canadainc::IOUtils::writeFile( q.value(LOCAL_PATH).toString(), data );
    return q;
}

QVariantMap processPlaylist(QString const& reciter, QString const& outputDirectory, QList< QPair<int,int> > const& playlist)
{
    QVariantMap result;

    QDir q( QString("%1/%2").arg(outputDirectory).arg(reciter) );
    if ( !q.exists() ) {
        q.mkpath(".");
    }

    if ( !q.exists() ) {
        result[PLAYLIST_ERROR] = QObject::tr("Quran10 does not seem to be able to write to the output folder. Please try selecting a different output folder or restart your device.");
        return result;
    }

    int n = playlist.size();
    QVariantList queue;
    QStringList toPlay;
    QSet<QString> alreadyQueued; // we maintain this to avoid putting duplicates in the queue (ie: during memorization mode)

    QString standardHost = qgetenv("HOST_RECITATION_STANDARD");

    for (int i = 0; i < n; i++)
    {
        QPair<int,int> track = playlist[i];
        bool isPage = track.second == 0;
        QString fileName = isPage ? QString("Page%1.mp3").arg( normalize(track.first) ) : QString("%1%2.mp3").arg( normalize(track.first) ).arg( normalize(track.second) );
        QString absolutePath = QString("%1/%2").arg( q.path() ).arg(fileName);

        if ( !QFile(absolutePath).exists() && !alreadyQueued.contains(absolutePath) )
        {
            QVariantMap q;
            q[URI_KEY] = QString(isPage ? "%1/%2/PageMp3s/%3" : "%1/%2/%3").arg(standardHost).arg(reciter).arg(fileName);
            q[LOCAL_PATH] = absolutePath;
            q[KEY_TRANSFER_NAME] = isPage ? QObject::tr("Page %1 recitation").arg(track.first) : QObject::tr("%1:%2 recitation").arg(track.first).arg(track.second);
            q[COOKIE_RECITATION_MP3] = true;

            if (track.second > 0)
            {
                q[KEY_CHAPTER] = track.first;
                q[KEY_VERSE] = track.second;
            }

            queue << q;
            alreadyQueued << absolutePath;
        }

        toPlay << absolutePath;
    }

    if ( !queue.isEmpty() )
    {
        result[KEY_QUEUE] = queue;
        result[ANCHOR_KEY] = queue.last().toMap().value(URI_KEY).toString();
    }

    if ( toPlay.size() > 1 )
    {
        bool written = !toPlay.isEmpty() ? IOUtils::writeTextFile( PLAYLIST_TARGET, toPlay.join("\n"),  true, true, "" ) : false;
        LOGGER(written);

        if (written) {
            result[PLAYLIST_KEY] = QUrl::fromLocalFile(PLAYLIST_TARGET);
        } else {
            result[PLAYLIST_ERROR] = QObject::tr("Quran10 could not write the playlist. Please try restarting your device.");
        }
    } else if ( toPlay.size() == 1 ) {
        result[PLAYLIST_KEY] = QUrl::fromLocalFile( toPlay.first() );
    }

    return result;
}

}

namespace quran {

using namespace canadainc;

RecitationHelper::RecitationHelper(QueueDownloader* queue, Persistance* p, QObject* parent) :
        QObject(parent), m_persistance(p), m_queue(queue)
{
    connect( queue, SIGNAL( requestComplete(QVariant const&, QByteArray const&) ), this, SLOT( onRequestComplete(QVariant const&, QByteArray const&) ) );
    connect( &m_futureResult, SIGNAL( finished() ), this, SLOT( onPlaylistReady() ) );
}


int RecitationHelper::extractIndex(QVariantMap const& m)
{
    if ( m_ayatToIndex.isEmpty() ) {
        return -1;
    }

    QString uri = m.value(URI_KEY).toString();
    uri = uri.mid( uri.lastIndexOf("/")+1 );
    uri = uri.left( uri.lastIndexOf(".") );
    int verse = uri.mid(3).toInt();
    int chapter = uri.left(3).toInt();

    return m_ayatToIndex.value( qMakePair<int,int>(chapter,verse) );
}


int RecitationHelper::extractPage(QVariantMap const& m)
{
    QString uri = m.value(URI_KEY).toString();
    uri = uri.mid( uri.lastIndexOf("/")+1 );
    uri = uri.left( uri.lastIndexOf(".") );
    return uri.mid(4).toInt();
}


void RecitationHelper::memorize(bb::cascades::ArrayDataModel* adm, int from, int to)
{
    LOGGER(adm->size() << from << to);

    if ( !m_futureResult.isRunning() )
    {
        m_ayatToIndex.clear();

        for (int i = from; i <= to; i++)
        {
            QVariantMap q = adm->value(i).toMap();
            QPair<int,int> ayat = qMakePair<int,int>( q.value(KEY_CHAPTER_ID).toInt(), q.value(KEY_VERSE_ID).toInt() );
            m_ayatToIndex.insert(ayat, i);
        }

        QList< QPair<int,int> > all;
        int k = 0;
        int fromVerse = from;

        while (k < 2)
        {
            int endPoint = from+CHUNK_SIZE;

            if (endPoint > to) {
                endPoint = to+1;
            }

            for (int verse = fromVerse; verse < endPoint; verse++)
            {
                for (int j = 0; j < ITERATION; j++)
                {
                    QVariantMap q = adm->value(verse).toMap();
                    all << qMakePair<int,int>( q.value(KEY_CHAPTER_ID).toInt(), q.value(KEY_VERSE_ID).toInt() );
                }
            }

            for (int j = 0; j < ITERATION; j++)
            {
                for (int verse = fromVerse; verse < endPoint; verse++)
                {
                    QVariantMap q = adm->value(verse).toMap();
                    all << qMakePair<int,int>( q.value(KEY_CHAPTER_ID).toInt(), q.value(KEY_VERSE_ID).toInt() );
                }
            }

            fromVerse += CHUNK_SIZE;

            if (fromVerse > endPoint) {
                break;
            }

            ++k;
        }

        for (int j = 0; j < ITERATION; j++)
        {
            for (int verse = from; verse < to; verse++)
            {
                QVariantMap q = adm->value(verse).toMap();
                all << qMakePair<int,int>( q.value(KEY_CHAPTER_ID).toInt(), q.value(KEY_VERSE_ID).toInt() );
            }
        }

        QFuture<QVariantMap> future = QtConcurrent::run(processPlaylist, m_persistance->getValueFor(KEY_RECITER).toString(), Persistance::hasSharedFolderAccess() ? m_persistance->getValueFor(KEY_OUTPUT_FOLDER).toString() : QDir::homePath(), all);
        m_futureResult.setFuture(future);
    }
}


void RecitationHelper::downloadAndPlay(int chapter, int verse)
{
    LOGGER(chapter << verse);

    if ( !m_futureResult.isRunning() )
    {
        QList< QPair<int,int> > all;
        all << qMakePair<int,int>(chapter, verse);

        QFuture<QVariantMap> future = QtConcurrent::run(processPlaylist, m_persistance->getValueFor(KEY_RECITER).toString(), Persistance::hasSharedFolderAccess() ? m_persistance->getValueFor(KEY_OUTPUT_FOLDER).toString() : QDir::homePath(), all);
        m_futureResult.setFuture(future);
    }
}


void RecitationHelper::onRequestComplete(QVariant const& cookie, QByteArray const& data)
{
    if ( cookie.toMap().contains(COOKIE_RECITATION_MP3) )
    {
        QFutureWatcher<QVariantMap>* qfw = new QFutureWatcher<QVariantMap>(this);
        connect( qfw, SIGNAL( finished() ), this, SLOT( onWritten() ) );

        QFuture<QVariantMap> future = QtConcurrent::run(writeVerse, cookie, data);
        qfw->setFuture(future);
    }
}


void RecitationHelper::onWritten()
{
    QFutureWatcher<QVariantMap>* qfw = static_cast< QFutureWatcher<QVariantMap>* >( sender() );
    QVariantMap result = qfw->result();

    if ( !m_anchor.isEmpty() && m_anchor == result.value(URI_KEY).toString() ) {
        startPlayback();
    }

    qfw->deleteLater();
}


void RecitationHelper::onPlaylistReady()
{
    QVariantMap result = m_futureResult.result();

    if ( result.contains(PLAYLIST_ERROR) )
    {
        LOGGER("PlaylistError");
        QString message = result.value(PLAYLIST_ERROR).toString();
        m_persistance->showToast( message, ASSET_YELLOW_DELETE );
        AppLogFetcher::getInstance()->record("PlaylistError", message);
    } else if ( result.contains(KEY_QUEUE) ) {
        QVariantList queue = result.value(KEY_QUEUE).toList();
        m_anchor = result.value(ANCHOR_KEY).toString();
        m_queue->process(queue);

        if ( result.contains(PLAYLIST_KEY) ) {
            m_playlistUrl = result.value(PLAYLIST_KEY).toUrl();
        }
    } else if ( result.contains(PLAYLIST_KEY) ) {
        m_playlistUrl = result.value(PLAYLIST_KEY).toUrl();
        startPlayback();
    }
}


void RecitationHelper::downloadAndPlayAll(bb::cascades::ArrayDataModel* adm, int from, int to)
{
    LOGGER( adm->size() << from << to );

    if ( !m_futureResult.isRunning() )
    {
        QList< QPair<int,int> > all;
        int n = to >= from ? to : adm->size()-1;
        m_ayatToIndex.clear();

        if ( from != 1 && from != 9 && m_persistance->getValueFor("playBismillah") == 1 ) { // if it's not Surah Al-Faatiha & at-Tawbah
            all << qMakePair<int,int>(1,1);
        }

        for (int i = from; i <= n; i++)
        {
            QVariantMap q = adm->value(i).toMap();
            QPair<int,int> ayat = qMakePair<int,int>( q.value(KEY_CHAPTER_ID).toInt(), q.value(KEY_VERSE_ID).toInt() );
            all << ayat;
            m_ayatToIndex.insert(ayat, i);
        }

        QFuture<QVariantMap> future = QtConcurrent::run(processPlaylist, m_persistance->getValueFor(KEY_RECITER).toString(), Persistance::hasSharedFolderAccess() ? m_persistance->getValueFor(KEY_OUTPUT_FOLDER).toString() : QDir::homePath(), all);
        m_futureResult.setFuture(future);
    }
}


void RecitationHelper::downloadAndPlayTajweed(int chapter, int verse)
{
    LOGGER(chapter << verse);

    static QMap<int, QString> chapterToPath;

    if ( chapterToPath.isEmpty() )
    {
        chapterToPath[1] = "Al Fatihah/fatihah";
        chapterToPath[95] = "At Tin/at-tin";
        chapterToPath[97] = "Al Qadr/alqadr";
        chapterToPath[99] = "Az Zalzalah/azzalzalah";
        chapterToPath[100] = "Al Adiyat/adiyat";
        chapterToPath[101] = "Al Qaria/qariah";
        chapterToPath[102] = "At Takathur/at";
        chapterToPath[103] = "Al Asr/asr";
        chapterToPath[104] = "Al Humaza/humazah";
        chapterToPath[105] = "Al Fil/fil";
        chapterToPath[106] = "Al Quraish/alquraish";
        chapterToPath[107] = "Al Maun/maum";
        chapterToPath[108] = "Al Kauthar/kauthar";
        chapterToPath[109] = "Al Kafirun/kafirun";
        chapterToPath[110] = "An Nasr/an-nasr";
        chapterToPath[111] = "Al Masad/almasad";
        chapterToPath[112] = "Al Ikhlas/ikhlas";
        chapterToPath[113] = "Al Falaq/falaq";
        chapterToPath[114] = "An Nas/al-nas";
    }

    if (chapter == 1 && verse != 7) {
        verse -= 1; // because basmalah is separate
    }

    QString fileName = QString("%1-ayah%2.mp3").arg( chapterToPath.value(chapter) ).arg(verse);

    if (chapter == 1 && verse == 0) {
        fileName = QString("%1-ayah0%2.mp3").arg( chapterToPath.value(chapter) ).arg(verse+1);
    }

    QDir q( QString("%1/tajweed").arg( Persistance::hasSharedFolderAccess() ? m_persistance->getValueFor(KEY_OUTPUT_FOLDER).toString() : QDir::homePath() ) );

    if ( !q.exists() ) {
        q.mkpath(".");
    }

    QString absolutePath = QString("%1/%2").arg( q.path() ).arg( fileName.mid( fileName.lastIndexOf("/")+1 ) );
    m_playlistUrl = QUrl::fromLocalFile(absolutePath);

    if ( QFile::exists(absolutePath) ) {
        startPlayback();
    } else {
        QVariantMap q;
        q[COOKIE_RECITATION_MP3] = true;
        q[URI_KEY] = QString("%1/%2").arg( QString( qgetenv("HOST_TAJWEED") ) ).arg(fileName);
        q[LOCAL_PATH] = absolutePath;
        q[KEY_TRANSFER_NAME] = QObject::tr("%1:%2 tajweed").arg(chapter).arg(verse);
        q[KEY_CHAPTER] = chapter;
        q[KEY_VERSE] = verse;
        m_anchor = q.value(URI_KEY).toString();

        m_queue->process( QVariantList() << q );
    }
}


bool RecitationHelper::tajweedAvailable(int chapter, int verse)
{
    Q_UNUSED(verse);

    static QSet<int> chaptersSupported;

    if ( chaptersSupported.isEmpty() )
    {
        chaptersSupported << 1;
        chaptersSupported << 95;
        chaptersSupported << 97;

        for (int i = 99; i <= 114; i++) {
            chaptersSupported << i;
        }
    }

    return chaptersSupported.contains(chapter);
}


void RecitationHelper::startPlayback() {
    emit readyToPlay(m_playlistUrl);
}


RecitationHelper::~RecitationHelper()
{
}

} /* namespace quran */
