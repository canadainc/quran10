#include "precompiled.h"

#include "QueryTafsirHelper.h"
#include "DatabaseHelper.h"
#include "Logger.h"
#include "QueryId.h"
#include "TextUtils.h"

namespace {

QString combine(QVariantList const& arabicIds)
{
    QStringList ids;

    foreach (QVariant const& entry, arabicIds) {
        ids << QString::number( entry.toInt() );
    }

    return ids.join(",");
}

QVariant protect(QString const& a) {
    return a.isEmpty() ? QVariant() : a;
}

}

namespace quran {

QueryTafsirHelper::QueryTafsirHelper(DatabaseHelper* sql) : m_sql(sql)
{
}


qint64 QueryTafsirHelper::addBio(QObject* caller, QString const& body, QString const& reference, QString const& author, QString const& heading)
{
    LOGGER( body.length() << reference.length() << author << heading );

    qint64 id = QDateTime::currentMSecsSinceEpoch();
    QVariantList args = QVariantList() << id << body << protect(reference) << protect(heading);

    if ( !author.isEmpty() ) {
        args << generateIndividualField(caller, author);
    } else {
        args << QVariant();
    }

    QString query = "INSERT INTO biographies (id,body,reference,heading,author) VALUES(?,?,?,?,?)";
    m_sql->executeQuery(caller, query, QueryId::AddBio, args);

    return id;
}


qint64 QueryTafsirHelper::addBioLink(QObject* caller, qint64 bioId, qint64 targetId, QVariant const& points)
{
    LOGGER(bioId << targetId << points);

    qint64 id = QDateTime::currentMSecsSinceEpoch();

    QString query = "INSERT INTO mentions (id,target,bio_id,points) VALUES(?,?,?,?)";
    m_sql->executeQuery(caller, query, QueryId::AddBioLink, QVariantList() << id << targetId << bioId << points);

    return id;
}


void QueryTafsirHelper::editBio(QObject* caller, qint64 bioId, QString const& body, QString const& reference, QString const& author, QString const& heading)
{
    LOGGER( bioId << body.length() << reference.length() << author << heading );

    QVariantList args = QVariantList() << body << protect(reference) << protect(heading);

    if ( !author.isEmpty() ) {
        args << generateIndividualField(caller, author);
    } else {
        args << QVariant();
    }

    QString query = QString("UPDATE biographies SET body=?, reference=?, heading=?, author=? WHERE id=%1").arg(bioId);
    m_sql->executeQuery(caller, query, QueryId::EditBio, args);
}


void QueryTafsirHelper::addWebsite(QObject* caller, qint64 individualId, QString const& address)
{
    QString query = QString("INSERT INTO websites (individual,uri) VALUES(%1,?)").arg(individualId);
    m_sql->executeQuery(caller, query, QueryId::AddWebsite, QVariantList() << address);
}


void QueryTafsirHelper::addLocation(QObject* caller, QString const& city, qreal latitude, qreal longitude)
{
    LOGGER(city << latitude << longitude);
    QString query = "INSERT INTO locations (city,latitude,longitude) VALUES(?,?,?)";
    m_sql->executeQuery(caller, query, QueryId::AddLocation, QVariantList() << city << latitude << longitude);
}


void QueryTafsirHelper::addQuote(QObject* caller, QString const& author, QString const& body, QString const& reference, qint64 suiteId, QString const& uri)
{
    LOGGER(author << body << reference << suiteId << uri);

    qint64 authorId = generateIndividualField(caller, author);
    QString query = QString("INSERT INTO quotes (author,body,reference,suite_id,uri) VALUES(%1,?,?,?,?)").arg(authorId);
    QVariantList args = QVariantList() << body << reference;
    args <<  (suiteId ? suiteId : QVariant() );
    args << protect(uri);

    m_sql->executeQuery(caller, query, QueryId::AddQuote, args);
}


void QueryTafsirHelper::addTafsir(QObject* caller, QString const& author, QString const& translator, QString const& explainer, QString const& title, QString const& description, QString const& reference)
{
    LOGGER(author << translator << explainer << title << description << reference);

    QStringList fields = QStringList() << "id" << "author" << "title" << "description" << "reference";
    QVariantList args = QVariantList() << QDateTime::currentMSecsSinceEpoch() << generateIndividualField(caller, author) << title << protect(description) << reference;

    if ( !translator.isEmpty() ) {
        fields << "translator";
        args << generateIndividualField(caller, translator);
    }

    if ( !explainer.isEmpty() ) {
        fields << "explainer";
        args << generateIndividualField(caller, explainer);
    }

    QString query = QString("INSERT OR IGNORE INTO suites (%1) VALUES(%2)").arg( fields.join(",") ).arg( TextUtils::getPlaceHolders( args.size(), false ) );
    m_sql->executeQuery(caller, query, QueryId::AddTafsir, args);
}


void QueryTafsirHelper::addTafsirPage(QObject* caller, qint64 suiteId, QString const& body, QString const& heading, QString const& reference)
{
    LOGGER( suiteId << body.length() << reference.length() );

    QString query = QString("INSERT OR IGNORE INTO suite_pages (id,suite_id,body,heading,reference) VALUES(%1,%2,?,?,?)").arg( QDateTime::currentMSecsSinceEpoch() ).arg(suiteId);
    m_sql->executeQuery(caller, query, QueryId::AddTafsirPage, QVariantList() << body << protect(heading) << protect(reference) );
}


void QueryTafsirHelper::addStudent(QObject* caller, qint64 teacherId, qint64 studentId)
{
    LOGGER(teacherId << studentId);

    QString query = QString("INSERT OR IGNORE INTO teachers(teacher,individual) VALUES(%1,%2)").arg(teacherId).arg(studentId);
    m_sql->executeQuery(caller, query, QueryId::AddStudent);
}


void QueryTafsirHelper::addTeacher(QObject* caller, qint64 studentId, qint64 teacherId)
{
    LOGGER(studentId << teacherId);

    QString query = QString("INSERT OR IGNORE INTO teachers(individual,teacher) VALUES(%1,%2)").arg(studentId).arg(teacherId);
    m_sql->executeQuery(caller, query, QueryId::AddTeacher);
}


void QueryTafsirHelper::editTafsir(QObject* caller, qint64 suiteId, QString const& author, QString const& translator, QString const& explainer, QString const& title, QString const& description, QString const& reference)
{
    LOGGER(suiteId << author << translator << explainer << title << description << reference);

    QStringList fields = QStringList() << "author=?" << "title=?" << "description=?" << "reference=?" << "translator=?" << "explainer=?";
    QVariantList args = QVariantList() << generateIndividualField(caller, author);
    args << title;
    args << protect(description);
    args << reference;

    if ( translator.isEmpty() ) {
        args << QVariant();
    } else {
        args << generateIndividualField(caller, translator);
    }

    if ( explainer.isEmpty() ) {
        args << QVariant();
    } else {
        args << generateIndividualField(caller, explainer);
    }

    QString query = QString("UPDATE suites SET %2 WHERE id=%1").arg(suiteId).arg( fields.join(",") );
    m_sql->executeQuery(caller, query, QueryId::EditTafsir, args);
}


void QueryTafsirHelper::editTafsirPage(QObject* caller, qint64 suitePageId, QString const& body, QString const& heading, QString const& reference)
{
    LOGGER( suitePageId << body.length() << heading.length() << reference.length() );

    QString query = QString("UPDATE suite_pages SET body=?, heading=?, reference=? WHERE id=%1").arg(suitePageId);
    m_sql->executeQuery( caller, query, QueryId::EditTafsirPage, QVariantList() << body << protect(heading) << protect(reference) );
}


void QueryTafsirHelper::editIndividual(QObject* caller, qint64 id, QString const& prefix, QString const& name, QString const& kunya, QString const& displayName, bool hidden, int birth, int death, bool female, int location, bool companion)
{
    LOGGER( id << prefix << name << kunya << displayName << hidden << birth << death << female << location );

    QString query = QString("UPDATE individuals SET prefix=?, name=?, kunya=?, displayName=?, hidden=?, birth=?, death=?, female=?, location=?, is_companion=? WHERE id=%1").arg(id);

    QVariantList args;
    args << protect(prefix);
    args << name;
    args << protect(kunya);
    args << protect(displayName);
    args << ( hidden ? 1 : QVariant() );
    args << ( birth != 0 ? birth : QVariant() );
    args << ( death != 0 ? death : QVariant() );
    args << ( female ? 1 : QVariant() );
    args << ( location > 0 ? location : QVariant() );
    args << ( companion ? 1 : QVariant() );

    m_sql->executeQuery(caller, query, QueryId::EditIndividual, args);
}


void QueryTafsirHelper::editQuote(QObject* caller, qint64 quoteId, QString const& author, QString const& body, QString const& reference, qint64 suiteId, QString const& uri)
{
    LOGGER(quoteId << author << body << reference << suiteId << uri);

    qint64 authorId = generateIndividualField(caller, author);
    QString query = QString("UPDATE quotes SET author=%2,body=?,reference=?,suiteId=?,uri=? WHERE id=%1").arg(quoteId).arg(authorId);
    QVariantList args = QVariantList() << body << reference;
    args << ( suiteId ? suiteId : QVariant() );
    args << protect(uri);

    m_sql->executeQuery(caller, query, QueryId::EditQuote, args);
}


void QueryTafsirHelper::editLocation(QObject* caller, qint64 id, QString const& city)
{
    LOGGER(id << city);

    QString query = QString("UPDATE locations SET city=? WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::EditLocation, QVariantList() << city);
}


void QueryTafsirHelper::fetchAllBios(QObject* caller) {
    m_sql->executeQuery(caller, QString("SELECT biographies.id AS bio_id,mentions.id AS mention_id,%1 AS author,%2 AS target,heading,body,reference,points FROM biographies LEFT JOIN mentions ON mentions.bio_id=biographies.id LEFT JOIN individuals i ON biographies.author=i.id LEFT JOIN individuals j ON mentions.target=j.id").arg( NAME_FIELD("i") ).arg( NAME_FIELD("j") ), QueryId::FetchAllBios);
}


void QueryTafsirHelper::fetchAllIndividuals(QObject* caller) {
    m_sql->executeQuery(caller, "SELECT individuals.id,prefix,name,kunya,hidden,birth,death,is_companion FROM individuals ORDER BY name,kunya,prefix", QueryId::FetchAllIndividuals);
}


void QueryTafsirHelper::fetchAllLocations(QObject* caller, QString const& city)
{
    LOGGER(city);
    QString q = "SELECT * FROM locations";

    QVariantList args;

    if ( !city.isEmpty() ) {
        q += " WHERE city LIKE '%' || ? || '%'";
        args << city;
    }

    q += " ORDER BY city";

    m_sql->executeQuery(caller, q, QueryId::FetchAllLocations, args);
}


void QueryTafsirHelper::fetchAllOrigins(QObject* caller)
{
    m_sql->executeQuery(caller, QString("SELECT %1 AS name,i.id,city,latitude,longitude FROM individuals i INNER JOIN locations ON i.location=locations.id WHERE i.hidden ISNULL").arg( NAME_FIELD("i") ), QueryId::FetchAllOrigins);
}


void QueryTafsirHelper::fetchBioMetadata(QObject* caller, qint64 bioId)
{
    LOGGER(bioId);
    m_sql->executeQuery(caller, QString("SELECT * FROM biographies WHERE id=%1").arg(bioId), QueryId::FetchBioMetadata);
}


void QueryTafsirHelper::fetchTeachers(QObject* caller, qint64 individualId)
{
    LOGGER(individualId);
    m_sql->executeQuery(caller, QString("SELECT i.id,%1 AS teacher FROM teachers INNER JOIN individuals i ON teachers.teacher=i.id WHERE teachers.individual=%2 AND i.hidden ISNULL").arg( NAME_FIELD("i") ).arg(individualId), QueryId::FetchTeachers);
}


void QueryTafsirHelper::fetchStudents(QObject* caller, qint64 individualId)
{
    LOGGER(individualId);
    m_sql->executeQuery(caller, QString("SELECT i.id,%1 AS student FROM teachers INNER JOIN individuals i ON teachers.individual=i.id WHERE teachers.teacher=%2 AND i.hidden ISNULL").arg( NAME_FIELD("i") ).arg(individualId), QueryId::FetchStudents);
}


void QueryTafsirHelper::fetchFrequentIndividuals(QObject* caller, int n)
{
    m_sql->executeQuery(caller, QString("SELECT author AS id,prefix,name,kunya,hidden,displayName,birth,death,is_companion FROM (SELECT author,COUNT(author) AS n FROM suites GROUP BY author UNION SELECT translator AS author,COUNT(translator) AS n FROM suites GROUP BY author UNION SELECT explainer AS author,COUNT(explainer) AS n FROM suites GROUP BY author ORDER BY n DESC LIMIT %1) INNER JOIN individuals ON individuals.id=author GROUP BY individuals.id ORDER BY name,kunya,prefix").arg(n), QueryId::FetchAllIndividuals);
}


void QueryTafsirHelper::fetchAllWebsites(QObject* caller, qint64 individualId)
{
    LOGGER(individualId);
    m_sql->executeQuery(caller, QString("SELECT id,uri FROM websites WHERE individual=%1 ORDER BY uri").arg(individualId), QueryId::FetchAllWebsites);
}


void QueryTafsirHelper::fetchAllTafsir(QObject* caller, qint64 individualId)
{
    LOGGER("fetchAllTafsir");

    QStringList queryParams = QStringList() << QString("SELECT suites.id AS id,%1 AS author,title FROM suites INNER JOIN individuals i ON i.id=suites.author").arg( NAME_FIELD("i") );

    if (individualId) {
        queryParams << QString("WHERE (author=%1 OR translator=%1 OR explainer=%1)").arg(individualId);
    }

    queryParams << "ORDER BY id DESC";

    m_sql->executeQuery(caller, queryParams.join(" "), QueryId::FetchAllTafsir);
}


void QueryTafsirHelper::fetchTafsirMetadata(QObject* caller, qint64 suiteId)
{
    LOGGER(suiteId);

    QString query = QString("SELECT author,translator,explainer,title,description,reference FROM suites WHERE id=%1").arg(suiteId);
    m_sql->executeQuery(caller, query, QueryId::FetchTafsirHeader);
}


void QueryTafsirHelper::fetchIndividualData(QObject* caller, qint64 individualId)
{
    LOGGER(individualId);

    QString query = QString("SELECT * FROM individuals WHERE id=%1").arg(individualId);
    m_sql->executeQuery(caller, query, QueryId::FetchIndividualData);
}


qint64 QueryTafsirHelper::generateIndividualField(QObject* caller, QString const& value)
{
    static QRegExp allNumbers = QRegExp("\\d+");

    if ( allNumbers.exactMatch(value) ) {
        return value.toLongLong();
    } else {
        qint64 id = QDateTime::currentMSecsSinceEpoch();

        if ( value.startsWith("Shaykh ") || value.startsWith("Sheikh ") || value.startsWith("Imam ") || value.startsWith("Imaam ") )
        {
            QStringList all = value.split(" ");
            QString prefix = all.takeFirst();
            QString actualName = all.join(" ");

            m_sql->executeQuery(caller, QString("INSERT INTO individuals (id,prefix,name) VALUES (%1,?,?)").arg(id), QueryId::AddIndividual, QVariantList() << prefix << actualName);
        } else {
            m_sql->executeQuery(caller, QString("INSERT INTO individuals (id,name) VALUES (%1,?)").arg(id), QueryId::AddIndividual, QVariantList() << value);
        }

        return id;
    }
}


void QueryTafsirHelper::createIndividual(QObject* caller, QString const& prefix, QString const& name, QString const& kunya, QString const& displayName, int birth, int death, int location, bool companion)
{
    LOGGER( prefix << name << kunya << displayName << birth << death << location );

    qint64 id = QDateTime::currentMSecsSinceEpoch();
    QString query = QString("INSERT INTO individuals (id,prefix,name,kunya,displayName,birth,death,location) VALUES (%1,?,?,?,?,?,?,?)").arg(id);

    QVariantList args;
    args << protect(prefix);
    args << name;
    args << protect(kunya);
    args << protect(displayName);
    args << ( birth != 0 ? birth : QVariant() );
    args << ( death != 0 ? death : QVariant() );
    args << ( location > 0 ? location : QVariant() );
    args << ( companion ? 1 : QVariant() );

    m_sql->executeQuery(caller, query, QueryId::AddIndividual, args);
}


void QueryTafsirHelper::linkAyatToTafsir(QObject* caller, qint64 suitePageId, int chapter, int fromVerse, int toVerse, QueryId::Type linkId)
{
    LOGGER(suitePageId << chapter << fromVerse << toVerse);
    QString query;

    if (chapter > 0)
    {
        if (fromVerse == 0) {
            query = QString("INSERT OR REPLACE INTO explanations (surah_id,suite_page_id) VALUES(%1,%2)").arg(chapter).arg(suitePageId);
        } else {
            query = QString("INSERT OR REPLACE INTO explanations (surah_id,from_verse_number,to_verse_number,suite_page_id) VALUES(%1,%2,%3,%4)").arg(chapter).arg(fromVerse).arg(toVerse).arg(suitePageId);
        }

        m_sql->executeQuery(caller, query, linkId);
    }
}


void QueryTafsirHelper::linkAyatsToTafsir(QObject* caller, qint64 suitePageId, QVariantList const& chapterVerseData)
{
    m_sql->startTransaction(caller, QueryId::LinkingAyatsToTafsir);

    foreach (QVariant const& q, chapterVerseData)
    {
        QVariantMap qvm = q.toMap();
        linkAyatToTafsir( caller, suitePageId, qvm.value(CHAPTER_KEY).toInt(), qvm.value(FROM_VERSE_KEY).toInt(), qvm.value(TO_VERSE_KEY).toInt() );
    }

    m_sql->endTransaction(caller, QueryId::LinkAyatsToTafsir);
}


void QueryTafsirHelper::removeBio(QObject* caller, qint64 id)
{
    LOGGER(id);
    QString query = QString("DELETE FROM biographies WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::RemoveBio);
}


void QueryTafsirHelper::removeBioLink(QObject* caller, qint64 id)
{
    LOGGER(id);
    QString query = QString("DELETE FROM mentions WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::RemoveBioLink);
}


void QueryTafsirHelper::removeQuote(QObject* caller, qint64 id)
{
    LOGGER(id);
    QString query = QString("DELETE FROM quotes WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::RemoveQuote);
}


void QueryTafsirHelper::removeWebsite(QObject* caller, qint64 id)
{
    LOGGER(id);
    QString query = QString("DELETE FROM websites WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::RemoveWebsite);
}


void QueryTafsirHelper::removeIndividual(QObject* caller, qint64 id)
{
    LOGGER(id);
    QString query = QString("DELETE FROM individuals WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::RemoveIndividual);
}


void QueryTafsirHelper::removeLocation(QObject* caller, qint64 id)
{
    LOGGER(id);
    QString query = QString("DELETE FROM locations WHERE id=%1").arg(id);
    m_sql->executeQuery(caller, query, QueryId::RemoveLocation);
}


void QueryTafsirHelper::removeTafsir(QObject* caller, qint64 suiteId)
{
    LOGGER(suiteId);

    QString query = QString("DELETE FROM suites WHERE id=%1").arg(suiteId);
    m_sql->executeQuery(caller, query, QueryId::RemoveTafsir);
}


void QueryTafsirHelper::removeTeacher(QObject* caller, qint64 individual, qint64 teacherId)
{
    LOGGER(individual << teacherId);

    QString query = QString("DELETE FROM teachers WHERE individual=%1 AND teacher=%2").arg(individual).arg(teacherId);
    m_sql->executeQuery(caller, query, QueryId::RemoveTeacher);
}


void QueryTafsirHelper::removeStudent(QObject* caller, qint64 individual, qint64 studentId)
{
    LOGGER(individual << studentId);

    QString query = QString("DELETE FROM teachers WHERE teacher=%1 AND individual=%2").arg(individual).arg(studentId);
    m_sql->executeQuery(caller, query, QueryId::RemoveStudent);
}


void QueryTafsirHelper::removeTafsirPage(QObject* caller, qint64 suitePageId)
{
    LOGGER(suitePageId);

    QString query = QString("DELETE FROM suite_pages WHERE id=%1").arg(suitePageId);
    m_sql->executeQuery(caller, query, QueryId::RemoveTafsirPage);
}


void QueryTafsirHelper::replaceIndividual(QObject* caller, qint64 toReplaceId, qint64 actualId)
{
    LOGGER(toReplaceId << actualId);

    m_sql->startTransaction(caller, QueryId::ReplacingIndividual);
    m_sql->executeQuery(caller, QString("UPDATE quotes SET author=%1 WHERE author=%2").arg(actualId).arg(toReplaceId), QueryId::ReplacingIndividual);
    m_sql->executeQuery(caller, QString("UPDATE suites SET author=%1 WHERE author=%2").arg(actualId).arg(toReplaceId), QueryId::ReplacingIndividual);
    m_sql->executeQuery(caller, QString("UPDATE suites SET translator=%1 WHERE translator=%2").arg(actualId).arg(toReplaceId), QueryId::ReplacingIndividual);
    m_sql->executeQuery(caller, QString("UPDATE suites SET explainer=%1 WHERE explainer=%2").arg(actualId).arg(toReplaceId), QueryId::ReplacingIndividual);
    m_sql->executeQuery(caller, QString("DELETE FROM individuals WHERE id=%1").arg(toReplaceId), QueryId::ReplacingIndividual);
    m_sql->endTransaction(caller, QueryId::ReplaceIndividual);
}


void QueryTafsirHelper::searchIndividuals(QObject* caller, QString const& trimmedText)
{
    LOGGER(trimmedText);
    m_sql->executeQuery(caller, QString("SELECT id,prefix,name,kunya,hidden,birth,death,is_companion FROM individuals i WHERE %1 ORDER BY name,kunya,prefix").arg( NAME_SEARCH("i") ), QueryId::SearchIndividuals, QVariantList() << trimmedText << trimmedText << trimmedText);
}


void QueryTafsirHelper::searchQuote(QObject* caller, QString fieldName, QString const& searchTerm)
{
    LOGGER(fieldName << searchTerm);

    QString query;
    QVariantList args = QVariantList() << searchTerm;

    if (fieldName == "author") {
        query = QString("SELECT quotes.id,%1 AS author,body,reference FROM quotes INNER JOIN individuals i ON i.id=quotes.author WHERE %2 ORDER BY quotes.id DESC").arg( NAME_FIELD("i") ).arg( NAME_SEARCH("i") );
        args << searchTerm << searchTerm;
    } else {
        query = QString("SELECT quotes.id,%2 AS author,body,reference FROM quotes INNER JOIN individuals i ON i.id=quotes.author WHERE %1 LIKE '%' || ? || '%' ORDER BY quotes.id DESC").arg(fieldName).arg( NAME_FIELD("i") );
    }

    m_sql->executeQuery(caller, query, QueryId::SearchQuote, args);
}


void QueryTafsirHelper::searchTafsir(QObject* caller, QString const& fieldName, QString const& searchTerm)
{
    LOGGER(fieldName << searchTerm);

    QString query;
    QVariantList args = QVariantList() << searchTerm;

    if (fieldName == "author" || fieldName == "explainer" || fieldName == "translator")
    {
        if (fieldName == "author") {
            query = QString("SELECT suites.id,%1 AS author,title FROM suites INNER JOIN individuals i ON i.id=suites.author WHERE %2 ORDER BY suites.id DESC").arg( NAME_FIELD("i") ).arg( NAME_SEARCH("i") );
            args << searchTerm << searchTerm;
        } else {
            query = QString("SELECT suites.id,%2 AS author,title FROM suites INNER JOIN individuals i ON i.id=suites.author INNER JOIN individuals t ON t.id=suites.%1 WHERE %3 ORDER BY suites.id DESC").arg(fieldName).arg( NAME_FIELD("i") ).arg( NAME_SEARCH("i") );
            args << searchTerm << searchTerm;
        }
    } else if (fieldName == "body") {
        query = QString("SELECT suites.id,%1 AS author,title FROM suites INNER JOIN individuals i ON i.id=suites.author INNER JOIN suite_pages ON suites.id=suite_pages.suite_id WHERE body LIKE '%' || ? || '%' ORDER BY suites.id DESC").arg( NAME_FIELD("i") );
    } else {
        query = QString("SELECT suites.id,%2 AS author,title FROM suites INNER JOIN individuals i ON i.id=suites.author WHERE %1 LIKE '%' || ? || '%' ORDER BY suites.id DESC").arg(fieldName).arg( NAME_FIELD("i") );
    }

    m_sql->executeQuery(caller, query, QueryId::SearchTafsir, args);
}


void QueryTafsirHelper::unlinkAyatsForTafsir(QObject* caller, QVariantList const& ids, qint64 suitePageId)
{
    LOGGER(ids << suitePageId);

    QString query = QString("DELETE FROM explanations WHERE id IN (%1) AND suite_page_id=%2").arg( combine(ids) ).arg(suitePageId);
    m_sql->executeQuery(caller, query, QueryId::UnlinkAyatsFromTafsir);
}


void QueryTafsirHelper::updateTafsirLink(QObject* caller, qint64 explanationId, int surahId, int fromVerse, int toVerse)
{
    LOGGER(explanationId << surahId << fromVerse << toVerse);

    QString query = QString("UPDATE explanations SET surah_id=%2,from_verse_number=%3,to_verse_number=%4 WHERE id=%1").arg(explanationId).arg(surahId).arg(fromVerse).arg(toVerse);
    m_sql->executeQuery(caller, query, QueryId::UpdateTafsirLink);
}


QueryTafsirHelper::~QueryTafsirHelper()
{
}

} /* namespace quran */
