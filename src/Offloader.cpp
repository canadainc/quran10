#include "precompiled.h"

#include "Offloader.h"
#include "CommonConstants.h"
#include "IOUtils.h"
#include "Logger.h"
#include "Persistance.h"
#include "QueueDownloader.h"
#include "TextUtils.h"
#include "ThreadUtils.h"
#include "ZipThread.h"

#define TAFSIR_MIME_IMAGE "imageSource"

namespace quran {

using namespace canadainc;

Offloader::Offloader(Persistance* persist, QObject* parent) : QObject(parent), m_persist(persist)
{
}


void Offloader::decorateSearchResults(QVariantList const& input, QString const& searchText, bb::cascades::ArrayDataModel* adm, QVariantList const& additional)
{
    LOGGER(input.size() << searchText << additional);

    QFutureWatcher<SimilarReference>* qfw = new QFutureWatcher<SimilarReference>(this);
    connect( qfw, SIGNAL( finished() ), this, SLOT( onResultsDecorated() ) );

    QFuture<SimilarReference> future = QtConcurrent::run(&ThreadUtils::decorateResults, input, adm, searchText, additional);
    qfw->setFuture(future);
}


void Offloader::decorateSimilarResults(QVariantList const& input, QString const& mainText, bb::cascades::ArrayDataModel* adm, bb::cascades::AbstractTextControl* atc)
{
    LOGGER(input.size() << mainText);

    QFutureWatcher<SimilarReference>* qfw = new QFutureWatcher<SimilarReference>(this);
    connect( qfw, SIGNAL( finished() ), this, SLOT( onResultsDecorated() ) );

    QFuture<SimilarReference> future = QtConcurrent::run(&ThreadUtils::decorateSimilar, input, adm, atc, mainText);
    qfw->setFuture(future);
}


void Offloader::onResultsDecorated()
{
    QFutureWatcher<SimilarReference>* qfw = static_cast< QFutureWatcher<SimilarReference>* >( sender() );
    ThreadUtils::onResultsDecorated( qfw->result() );

    sender()->deleteLater();
}


void Offloader::decorateTafsir(bb::cascades::ArrayDataModel* adm)
{
    for (int i = adm->size()-1; i >= 0; i--)
    {
        QVariantMap qvm = adm->value(i).toMap();
        QString body = qvm.value("body").toString();

        if ( body.endsWith("mp3") ) {
            qvm[TAFSIR_MIME_IMAGE] = "images/list/mime_mp3.png";
        } else if ( body.endsWith("doc") || body.endsWith("docx") ) {
            qvm[TAFSIR_MIME_IMAGE] = "images/list/mime_doc.png";
        } else if ( body.endsWith("pdf") ) {
            qvm[TAFSIR_MIME_IMAGE] = "images/list/mime_pdf.png";
        } else {
            qvm[TAFSIR_MIME_IMAGE] = "images/list/ic_tafsir.png";
        }

        adm->replace(i, qvm);
    }
}


void Offloader::backup(QString const& destination)
{
    LOGGER(destination);

    QFutureWatcher<QString>* qfw = new QFutureWatcher<QString>(this);
    connect( qfw, SIGNAL( finished() ), this, SLOT( onBookmarksSaved() ) );

    QFuture<QString> future = QtConcurrent::run(&ThreadUtils::compressBookmarks, destination);
    qfw->setFuture(future);
}


void Offloader::onBookmarksSaved()
{
    QFutureWatcher<QString>* qfw = static_cast< QFutureWatcher<QString>* >( sender() );
    QString result = qfw->result();

    emit backupComplete(result);

    qfw->deleteLater();
}


void Offloader::restore(QString const& source)
{
    LOGGER(source);

    QFutureWatcher<bool>* qfw = new QFutureWatcher<bool>(this);
    connect( qfw, SIGNAL( finished() ), this, SLOT( onBookmarksRestored() ) );

    QFuture<bool> future = QtConcurrent::run(&ThreadUtils::performRestore, source);
    qfw->setFuture(future);
}


void Offloader::onBookmarksRestored()
{
    QFutureWatcher<bool>* qfw = static_cast< QFutureWatcher<bool>* >( sender() );
    bool result = qfw->result();

    LOGGER("RestoreResult" << result);
    emit restoreComplete(result);

    qfw->deleteLater();
}


QString Offloader::textualizeAyats(bb::cascades::DataModel* adm, QVariantList const& selectedIndices, QString const& chapterTitle, bool showTranslation)
{
    QVariantMap first = adm->data( selectedIndices.first().toList() ).toMap();
    QVariantMap last = adm->data( selectedIndices.last().toList() ).toMap();
    int firstChapter = first.value("surah_id").toInt();
    int lastChapter = last.value("surah_id").toInt();
    int firstVerse = first.value("verse_id").toInt();
    int lastVerse = last.value("verse_id").toInt();

    QString footer;
    QStringList ayats;
    QStringList translations;

    if (firstChapter == lastChapter)
    {
        if (firstVerse == lastVerse) {
            footer = tr("%1 %2:%3").arg(chapterTitle).arg(firstChapter).arg(firstVerse);
        } else {
            footer = tr("%1 %2:%3-%4").arg(chapterTitle).arg(firstChapter).arg(firstVerse).arg(lastVerse);
        }
    } else {
        footer = tr("%1:%2-%3:%4").arg(firstChapter).arg(firstVerse).arg(lastChapter).arg(lastVerse);
    }

    int n = selectedIndices.size();

    for (int i = 0; i < n; i++)
    {
        QVariantMap current = adm->data( selectedIndices[i].toList() ).toMap();
        ayats << current.value("arabic").toString();

        if (showTranslation) {
            translations << current.value("translation").toString();
        }
    }

    return ayats.join("\n")+"\n\n"+translations.join("\n")+"\n\n"+footer;
}


qint64 Offloader::getFreeSpace()
{
    bb::FileSystemInfo fs;
    qint64 free = fs.availableFileSystemSpace( m_persist->getValueFor(KEY_OUTPUT_FOLDER).toString() );
    LOGGER("FreeSpace" << free << fs.errorString() << fs.error() );
    return free;
}


void Offloader::addToHomeScreen(int chapter, int verse, QString const& label)
{
    LOGGER(chapter << verse << label);

    bool added = bb::platform::HomeScreen().addShortcut( QString("asset:///images/menu/ic_mushaf.png"), TextUtils::sanitize(label), QString("quran://%1/%2").arg(chapter).arg(verse) );
    QString toastMessage = tr("Added %1 to home screen").arg(label);
    QString icon = "asset:///images/menu/ic_home.png";

    if (!added) {
        toastMessage = tr("Could not add %1 to home screen").arg(label);
        icon = "asset:///images/toast/yellow_delete.png";
    }

    m_persist->showToast(toastMessage, "", icon);
}


void Offloader::addToHomeScreen(qint64 suitePageId, QString const& label)
{
    LOGGER(suitePageId << label);

    bool added = bb::platform::HomeScreen().addShortcut( QString("asset:///images/list/ic_tafsir.png"), TextUtils::sanitize(label), QString("quran://tafsir/%1").arg(suitePageId) );
    QString toastMessage = tr("Added %1 to home screen").arg(label);
    QString icon = "asset:///images/menu/ic_home.png";

    if (!added) {
        toastMessage = tr("Could not add %1 to home screen").arg(label);
        icon = "asset:///images/toast/yellow_delete.png";
    }

    m_persist->showToast(toastMessage, "", icon);
}


QVariantList Offloader::removeOutOfRange(QVariantList input, int fromChapter, int fromVerse, int toChapter, int toVerse) {
    return ThreadUtils::removeOutOfRange(input, fromChapter, fromVerse, toChapter, toVerse);
}


QVariantList Offloader::normalizeJuzs(QVariantList const& source) {
    return ThreadUtils::normalizeJuzs(source);
}


QVariantList Offloader::computeNecessaryUpdates(QVariantMap const& q, QByteArray const& data)
{
    QVariantList downloadQueue;

    QStringList result = QString(data).split(",");
    QVariantMap requestData = q.value(KEY_UPDATE_CHECK).toMap();
    bool forcedUpdate = requestData.value(KEY_FORCED_UPDATE).toBool();

    if ( result.size() >= 6 )
    {
        qint64 serverTafsirVersion = result.first().toLongLong();
        qint64 myTafsirVersion = m_persist->getValueFor(KEY_TAFSIR_VERSION).toLongLong();
        qint64 serverTafsirSize = result[1].toLongLong();

        qint64 serverTranslationVersion = result[3].toLongLong();
        qint64 myTranslationVersion = m_persist->getValueFor(KEY_TRANSLATION_VERSION).toLongLong();
        qint64 serverTranslationSize = result[4].toLongLong();
        bool tafsirUpdateNeeded = serverTafsirVersion > myTafsirVersion;
        bool translationUpdateNeeded = serverTranslationVersion > myTranslationVersion;

        QString message;

        if (tafsirUpdateNeeded && translationUpdateNeeded)
        {
            if (forcedUpdate) {
                message = tr("Quran10 needs to download and install translation and tafsir files. The total size is ~%1. Do you want to download them now? If you say No you can download them at a later time but the app will not function as expected in the meantime!").arg( TextUtils::bytesToSize(serverTafsirSize+serverTranslationSize) );
            } else {
                message = tr("There are newer translation and tafsir files available. The total download size is ~%1. Do you want to download them now? If you say No you can download them at a later time.").arg( TextUtils::bytesToSize(serverTafsirSize+serverTranslationSize) );
            }
        } else if (tafsirUpdateNeeded) {
            if (forcedUpdate) {
                message = tr("Quran10 needs to download and install tafsir files. The total size is ~%1. Do you want to download it now? If you say No you can download it at a later time but the app will not function as expected in the meantime!").arg( TextUtils::bytesToSize(serverTafsirSize) );
            } else {
                message = tr("There are newer tafsir files available. The total download size is ~%1. Do you want to download it now? If you say No you can download it at a later time.").arg( TextUtils::bytesToSize(serverTafsirSize) );
            }
        } else if ( translationUpdateNeeded && requestData.contains(KEY_TRANSLATION) ) {
            if (forcedUpdate) {
                message = tr("Quran10 needs to download and install translation files. The total size is ~%1. Do you want to download it now? If you say No you can download it at a later time but the app will not function as expected in the meantime!").arg( TextUtils::bytesToSize(serverTranslationSize) );
            } else {
                message = tr("There are newer translation files available. The total download size is ~%1. Do you want to download it now? If you say No you can download it at a later time.").arg( TextUtils::bytesToSize(serverTranslationSize) );
            }
        } else {
            m_persist->saveValueFor( KEY_LAST_UPDATE, QDateTime::currentMSecsSinceEpoch(), false );
        }

        if ( !message.isNull() )
        {
            bool rememberMeValue = false;
            bool agreed = m_persist->getValueFor(KEY_UPDATE_CHECK_FLAG).toInt() == ALWAYS_UPDATE_FLAG;

            if (!agreed) {
                agreed = Persistance::showBlockingDialog( tr("Updates"), message, !forcedUpdate ? tr("Don't ask again") : "", rememberMeValue, tr("Yes"), tr("No") );
            }

            if (!agreed && rememberMeValue) { // don't update, and don't ask again
                m_persist->saveValueFor(KEY_UPDATE_CHECK_FLAG, -1, false);
            } else if (agreed && rememberMeValue) {
                m_persist->saveValueFor(KEY_UPDATE_CHECK_FLAG, -1, false);
            }

            if (agreed)
            {
                if (tafsirUpdateNeeded)
                {
                    QString serverTafsirMd5 = result[2];
                    QString tafsirName = requestData.value(KEY_TAFSIR).toString();

                    QVariantMap q;
                    q["name"] = tr("Tafsir");
                    q[URI_KEY] = QString("%1/tafsir/%2.zip").arg(REMOTE_FOLDER).arg(tafsirName);
                    q[TAFSIR_PATH] = tafsirName;
                    q[KEY_MD5] = serverTafsirMd5;
                    q[KEY_PLUGIN_VERSION_KEY] = KEY_TAFSIR_VERSION;
                    q[KEY_PLUGIN_VERSION_VALUE] = serverTafsirVersion;

                    downloadQueue << q;
                }

                if (translationUpdateNeeded)
                {
                    QString serverTranslationMd5 = result[5];
                    QString language = requestData.value(KEY_TRANSLATION).toString();

                    QVariantMap q;
                    q["name"] = tr("Translation");
                    q[URI_KEY] = QString("%1/translations/%2.zip").arg(REMOTE_FOLDER).arg(language);
                    q[KEY_TRANSLATION] = language;
                    q[KEY_MD5] = serverTranslationMd5;
                    q[KEY_PLUGIN_VERSION_KEY] = KEY_TRANSLATION_VERSION;
                    q[KEY_PLUGIN_VERSION_VALUE] = serverTranslationVersion;

                    downloadQueue << q;
                }
            }
        }
    } else if (forcedUpdate) { // if this is a mandatory update, then show error message
        m_persist->showToast( tr("There is a problem communicating with the server so the app cannot download the necessary files just yet. Please try opening the app again later and it should automatically try the update again..."), "", "asset:///images/toast/ic_offline.png" );
    }

    return downloadQueue;
}


void Offloader::processDownloadedPlugin(QVariantMap const& q, QByteArray const& data)
{
    if ( q.contains(TAFSIR_PATH) ) {
        QFutureWatcher<QVariantMap>* qfw = new QFutureWatcher<QVariantMap>(this);
        connect( qfw, SIGNAL( finished() ), this, SLOT( onArchiveWritten() ) );

        QFuture<QVariantMap> future = QtConcurrent::run(&ThreadUtils::writePluginArchive, q, data, QString(TAFSIR_PATH));
        qfw->setFuture(future);
    } else if ( q.contains(KEY_TRANSLATION) ) {
        QFutureWatcher<QVariantMap>* qfw = new QFutureWatcher<QVariantMap>(this);
        connect( qfw, SIGNAL( finished() ), this, SLOT( onArchiveWritten() ) );

        QFuture<QVariantMap> future = QtConcurrent::run(&ThreadUtils::writePluginArchive, q, data, QString(KEY_TRANSLATION));
        qfw->setFuture(future);
    }
}


void Offloader::onArchiveWritten()
{
    QFutureWatcher<QVariantMap>* qfw = static_cast< QFutureWatcher<QVariantMap>* >( sender() );
    QVariantMap result = qfw->result();

    if ( !result.isEmpty() )
    {
        ZipThread* zt = new ZipThread( result.value(KEY_ARCHIVE_PATH).toString() );
        zt->metadata = result;
        connect( zt, SIGNAL( done(bool, QString const&, QVariantMap const&) ), this, SLOT( onArchiveDeflated(bool, QString const&, QVariantMap const&) ) );
        connect( zt, SIGNAL( deflationProgress(qint64, qint64) ), this, SIGNAL( archiveDeflationProgress(qint64, qint64) ) );

        IOUtils::startThread(zt);
    } else {
        m_persist->showToast( tr("Could not prepare the plugin for extraction. Please swipe-down from the top-bezel and file a bug report!"), "", "asset:///images/toast/ic_duplicate_replace.png" );
        LOGGER("CouldNotWriteArchiveToTemp!");
    }

    sender()->deleteLater();
}


void Offloader::onArchiveDeflated(bool success, QString const& error, QVariantMap const& result)
{
    LOGGER(success << error);
    QString pluginVersionKey = result.value(KEY_PLUGIN_VERSION_KEY).toString();
    QString pluginVersionValue = result.value(KEY_PLUGIN_VERSION_VALUE).toString();

    m_persist->saveValueFor(pluginVersionKey, pluginVersionValue, false);
    m_persist->showToast( tr("Successfully saved plugin"), "", "asset:///images/menu/ic_select_more_chapters.png" );

    emit deflationDone(success, error);
}


Offloader::~Offloader()
{
}

} /* namespace quran */