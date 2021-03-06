import bb.cascades 1.2
import com.canadainc.data 1.0

NavigationPane
{
    id: navigationPane
    
    onCreationCompleted: {
        Qt.navigationPane = navigationPane;
    }
    
    onPopTransitionEnded: {
        deviceUtils.cleanUpAndDestroy(page);

        if ( tutorial.promptVideo("http://youtu.be/YOXtjnNWVZM") ) {}
    }
    
    function onAyatPicked(surahId, verseId)
    {
        var ayatPage = Qt.launch("AyatPage.qml");
        ayatPage.surahId = surahId;
        ayatPage.verseId = verseId;
    }
    
    function onOpenChapter(surahId)
    {
        var p = Qt.launch("ChapterTafsirPicker.qml");
        p.chapterNumber = surahId;
    }
    
    SurahPickerPage
    {
        id: pickerPage
        showJuz: true

        titleBarSpace: Button
        {
            id: buttonControl
            property variant progressData
            text: progressData ? progressData.surah_id+":"+progressData.verse_id : ""
            imageSource: "images/dropdown/saved_bookmark.png"
            verticalAlignment: VerticalAlignment.Center
            maxWidth: tutorial.du(18.75)
            translationX: -250
            scaleX: 1.1
            scaleY: 1.1
            visible: false
            
            onClicked: {
                console.log("UserEvent: SavedBookmarkClicked");
                pickerPage.picked(progressData.surah_id, progressData.verse_id);
            }
            
            onVisibleChanged: {
                if ( visible && tutorial.isTopPane(navigationPane, pickerPage) ) {
                    tutorial.exec( "bookmarkAnchor", qsTr("Notice the button on the top left. This is used to track your Qu'ran reading progress. You can use it to quickly jump to the verse you last left off."), HorizontalAlignment.Left, VerticalAlignment.Top, tutorial.du(2), 0, tutorial.du(4) );
                }
                
                if (visible && scaleX != 1) {
                    rotator.play();
                }
            }
            
            function onDataLoaded(id, data)
            {
                if (id == QueryId.FetchLastProgress)
                {
                    if (data.length > 0)
                    {
                        buttonControl.progressData = data[0];
                        buttonControl.visible = true;
                    } else if ( persist.contains("bookmarks") ) {
                        bookmarkHelper.saveLegacyBookmarks( buttonControl, persist.getValueFor("bookmarks") );
                    }
                } else if (id == QueryId.SaveLegacyBookmarks) {
                    persist.remove("bookmarks");
                    persist.showToast( qsTr("Ported legacy bookmarks!"), "asset:///images/menu/ic_bookmark_add.png");
                }
            }
            
            function onLastPositionUpdated() {
                bookmarkHelper.fetchLastProgress(buttonControl);
            }
            
            contextActions: [
                ActionSet {
                    title: buttonControl.text
                    subtitle: buttonControl.progressData ? Qt.formatDateTime(buttonControl.progressData.timestamp) : ""
                }
            ]
            
            animations: [
                SequentialAnimation
                {
                    id: rotator
                    
                    TranslateTransition
                    {
                        fromX: -250
                        toX: 0
                        easingCurve: StockCurve.QuinticOut
                        duration: 750
                    }
                    
                    RotateTransition {
                        fromAngleZ: 360
                        toAngleZ: 0
                        easingCurve: StockCurve.ExponentialOut
                        duration: 750
                    }
                    
                    ScaleTransition
                    {
                        fromX: 1.1
                        fromY: 1.1
                        toX: 1
                        toY: 1
                        duration: 750
                        easingCurve: StockCurve.DoubleElasticOut
                    }
                }
            ]
        }

        pickerList.onSelectionChanged: {
            if (sortValue == "juz" && indexPath.length == 1) { // don't allow selection of headers
                pickerList.select(indexPath, false);
            } else {
                var all = pickerList.selectionList();
                var n = all.length;
                compareAction.enabled = n > 1 && n < 5;
                openAction.enabled = n > 0 && (sortValue == "juz" || sortValue == "normal");
                pickerList.multiSelectHandler.status = qsTr("%n chapters selected", "", n);
            }
        }
        
        pickerList.multiSelectAction: MultiSelectActionItem {
            imageSource: "images/menu/ic_select_more_chapters.png"
        }
        
        pickerList.multiSelectHandler.onActiveChanged: {
            if (!active) {
                pickerList.clearSelection();
            } else {
                tutorial.execActionBar("compare", qsTr("Use the '%1' action to compare two or more surahs side by side. A maximum of 4 surahs may be compared at once.").arg(compareAction.title), "l", true );
                tutorial.execActionBar("openRange", qsTr("Use the '%1' action to open all the surah between the first selection and the last selection.").arg(openAction.title), "r", true);
                
                if (!openAction.enabled) {
                    tutorial.execActionBar("openRangeDisabled", qsTr("Note that the '%1' action is only available in the '%2' and '%3' display modes.").arg(openAction.title).arg(pickerPage.normalMode).arg(pickerPage.juzMode), "r", true);
                }
            }
        }

        pickerList.multiSelectHandler.actions: [
            ActionItem
            {
                id: compareAction
                enabled: false
                imageSource: "images/menu/ic_compare.png"
                title: qsTr("Compare") + Retranslate.onLanguageChanged
                
                onTriggered: {
                    console.log("UserEvent: CompareSurahs");
                    var p = Qt.launch("CompareSurahsPage.qml");
                    
                    var all = pickerPage.pickerList.selectionList();
                    var surahIds = [];
                    
                    for (var i = all.length-1; i >= 0; i--) {
                        var element = pickerPage.pickerList.dataModel.data(all[i]);
                        surahIds.push( {'surah_id': element.surah_id, 'verse_number': element.verse_number} );
                    }
                    
                    p.surahIds = surahIds;
                    
                    reporter.record("CompareSurahs", surahIds.toString());
                }
            },
            
            ActionItem
            {
                id: openAction
                enabled: false
                imageSource: "images/menu/ic_open_range.png"
                title: qsTr("Open Range") + Retranslate.onLanguageChanged
                
                onTriggered: {
                    console.log("UserEvent: OpenSurahs");
                    var p = Qt.launch("SurahPage.qml");
                    p.picked.connect(onAyatPicked);
                    p.openChapterTafsir.connect(onOpenChapter);
                    
                    var all = pickerPage.pickerList.selectionList();
                    var lastSelection = all[all.length-1];
                    var dm = pickerPage.pickerList.dataModel;
                    
                    p.fromSurahId = dm.data(all[0]).surah_id;
                    p.toSurahId = dm.data(lastSelection).surah_id;
                    
                    if (pickerPage.sortValue == "juz")
                    {
                        var nextIndex = dm.after(lastSelection);

                        if (nextIndex.length > 0)
                        {
                            var nextElement = dm.data(nextIndex);
                            
                            if (nextElement.surah_id == p.toSurahId) { // if it's the same surah spanning multiple juz
                                p.toVerseId = nextElement.verse_number-1;
                            }
                        }
                    }
                    
                    p.loadAyats();
                    
                    reporter.record("OpenSurahs", p.fromSurahId+"-"+p.toSurahId);
                }
            }
        ]
        
        actions: [
            ActionItem {
                title: qsTr("Mushaf") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_mushaf.png"
                ActionBar.placement: 'Signature' in ActionBarPlacement ? ActionBarPlacement["Signature"] : ActionBarPlacement.OnBar
                
                onTriggered: {
                    console.log("UserEvent: LaunchMushaf");
                    var sheet = Qt.initQml("MushafSheet.qml");
                    sheet.open();
                    
                    reporter.record("LaunchMushaf");
                }
                
                shortcuts: [
                    Shortcut {
                        key: qsTr("M") + Retranslate.onLanguageChanged
                    }
                ]
            },
            
            ActionItem
            {
                id: selectAll
                title: qsTr("Select All") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_select_all.png"
                enabled: pickerPage.sortValue != "juz"
                ActionBar.placement: ActionBarPlacement.OnBar
                
                onTriggered: {
                    console.log("UserEvent: SelectAllSurahs");
                    pickerPage.pickerList.multiSelectHandler.active = true;
                    pickerPage.pickerList.selectAll();
                    
                    reporter.record("SelectAllSurahs");
                }
                
                shortcuts: [
                    Shortcut {
                        key: qsTr("A") + Retranslate.onLanguageChanged
                    }
                ]
            }
        ]
        
        function createAndAttach(p)
        {
            var surahPage = Qt.launch(p);
            surahPage.picked.connect(onAyatPicked);
            surahPage.openChapterTafsir.connect(onOpenChapter);
            
            return surahPage;
        }
        
        onJuzPicked: {
            var surahPage = createAndAttach("JuzPage.qml");
            surahPage.juzId = juzId;
        }
        
        onPicked: {
            var surahPage = createAndAttach("SurahPage.qml");
            surahPage.surahId = chapter;
            surahPage.verseId = verse;
        }
        
        function onDataLoaded(id, data)
        {
            if (id == QueryId.FetchRandomQuote && data.length > 0)
            {
                var quote = data[0];
                var plainText = "“%1” - %2 [%3]".arg(quote.body).arg(quote.author).arg(quote.reference);
                
                var partQuote = "<i>“%1”</i>".arg( searchDecorator.toHtmlEscaped(quote.body) );
                var partAuthor = "<b>%1%2</b>".arg( searchDecorator.toHtmlEscaped(quote.author) ).arg( global.getSuffix(quote.birth, quote.death, quote.is_companion == 1, quote.female == 1) );
                var partSource = "[%1]".arg( searchDecorator.toHtmlEscaped(quote.reference) );
                var parts = "%1\n\n- %2\n\n%3".arg(partQuote).arg(partAuthor).arg(partSource);
                
                if (quote.translator) {
                    parts += "\n\nTranslated by <i>%1%2</i>".arg( searchDecorator.toHtmlEscaped(quote.translator) ).arg( global.getSuffix(quote.translator_birth, quote.translator_death, quote.translator_companion == 1, quote.translator_female == 1) );
                    plainText += "\n\nTranslated by "+quote.translator;
                }
                
                var body = "<html>"+parts+"</html>";
                
                notification.init(body, "images/list/ic_quote.png", plainText);
            }
        }
    }
    
    function onLazyInitComplete()
    {
        pickerPage.ready();
        
        tutorial.execActionBar( "openMushaf", qsTr("Tap here to open the mushaf!") );
        tutorial.execActionBar("selectAllSurahs", qsTr("Tap on the '%1' action to view the entire Qu'ran (all the surahs)!").arg(selectAll.title), "r");
        tutorial.exec("lpSurahPicker", "Press and hold on a surah for a menu to select multiple chapters.", HorizontalAlignment.Center, VerticalAlignment.Center, tutorial.du(2), 0, 0, tutorial.du(2));
        tutorial.expandOverflow("quranPane");
        
        buttonControl.onLastPositionUpdated();
        global.lastPositionUpdated.connect(buttonControl.onLastPositionUpdated);
        
        if (!selectAll.enabled) {
            tutorial.execActionBar("selectAllDisabled", qsTr("The '%1' feature is not available for the Juz display mode.").arg(selectAll.title), "r");
        }
        
        if ( !tutorial.active && persist.getValueFor("hideRandomQuote") != 1 ) {
            helper.fetchRandomQuote(pickerPage);
        }
    }
    
    attachedObjects: [
        SearchDecorator {
            id: searchDecorator
        }
    ]
}