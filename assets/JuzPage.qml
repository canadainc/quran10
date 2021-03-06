import bb.cascades 1.0
import com.canadainc.data 1.0

Page
{
    id: juzPage
    property int juzId
    property variant ranges
    actionBarAutoHideBehavior: ActionBarAutoHideBehavior.HideOnScroll
    signal picked(int surahId, int verseId)
    signal openChapterTafsir(int surahId)

    onJuzIdChanged:
    {
        busy.delegateActive = true;
        helper.fetchJuzInfo(juzPage, juzId);
    }
    
    onRangesChanged: {
        if (ranges)
        {
            awaker.lv.chapterNumber = ranges.from_surah_id;
            ctb.chapterNumber = ranges.from_surah_id;
            helper.fetchAllAyats(juzPage, ranges.from_surah_id, ranges.to_surah_id);
        }
    }
    
    onPeekedAtChanged: {
        awaker.lv.secretPeek = peekedAt;
    }
    
    function onDataLoaded(id, data)
    {
        if (id == QueryId.FetchAllAyats)
        {
            data = offloader.removeOutOfRange(data, ranges.from_surah_id, ranges.from_verse_id, ranges.to_surah_id, ranges.to_verse_id);
            
            awaker.lv.theDataModel.clear();
            awaker.lv.theDataModel.append(data);
            busy.delegateActive = false;
        } else if (id == QueryId.FetchJuz) {
            var toChapter = 114;
            var toVerse = 300;
            
            if (data.length > 1) {
                toChapter = data[1].surah_id;
                toVerse = data[1].verse_number;
            }
            
            ranges = {'from_surah_id': data[0].surah_id, 'from_verse_id': data[0].verse_number, 'to_surah_id': toChapter, 'to_verse_id': toVerse};
        }
    }
    
    function reload() {
        rangesChanged();
    }
    
    function cleanUp()
    {
        awaker.cleanUp();
        ctb.cleanUp();
        helper.textualChange.disconnect(reload);
    }

    onCreationCompleted: {
        helper.textualChange.connect(reload);
        mainContainer.add(awaker.lv);
    }
    
    titleBar: ChapterTitleBar
    {
        id: ctb
        scrollBehavior: TitleBarScrollBehavior.Sticky
        
        onNavigationTapped: {
            if (right) {
                ++juzId;
            } else {
                --juzId;
            }
            
            awaker.lazyPlayer.stop();
        }
        
        onTitleTapped: {
            var p = Qt.launch("ChapterTafsirPicker.qml")
            p.chapterNumber = chapterNumber;
        }
    }

    Container
    {
        layout: DockLayout {}
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.White
        
        Container
        {
            id: mainContainer
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
        }
        
        ProgressControl
        {
            id: busy
            asset: "images/progress/loading_surah.png"
        }
    }
    
    attachedObjects: [
        Awaker
        {
            id: awaker
            parentPage: juzPage
            
            onVersePicked: {
                picked(surahId, verseId);
            }
        }
    ]
}