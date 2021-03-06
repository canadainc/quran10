import QtQuick 1.0
import bb.cascades 1.2
import com.canadainc.data 1.0

ListView
{
    id: listView
    property alias theDataModel: verseModel
    property alias activeDefinition: activeDef
    property int chapterNumber
    property bool disableSpacing: helper.disableSpacing
    property int translationSize: helper.translationSize
    property int primarySize: helper.primarySize
    property int previousPlayedIndex
    property bool secretPeek: false
    property bool follow
    property bool showContextMenu: true
    property bool scrolled: false
    property bool blockPeek: false
    property bool showImages
    scrollRole: ScrollRole.Main

    dataModel: ArrayDataModel {
        id: verseModel
    }
    
    function cleanUp()
    {
        rangeSelector.cleanUp();
        player.metaDataChanged.disconnect(onMetaDataChanged);
        player.playbackCompleted.disconnect(clearPrevious);
    }
    
    onShowImagesChanged: {
        refresh();
    }
    
    onScrolledChanged: {
        if (scrolled) {
            timer.restart();
        }
    }
    
    leadingVisual: BismillahControl {
        delegateActive: chapterNumber > 1 && chapterNumber != 9
        sizeValue: primarySize
    }
    
    function play(from, to)
    {
        clearPrevious();
        previousPlayedIndex = -1;
        recitation.downloadAndPlayAll(verseModel, from, to);
        
        reporter.record("PlayFrom", verseModel.value(from).surah_id+":"+verseModel.value(from).verse_id)+","+to;
    }
    
    onSelectionChanged: {
        var n = selectionList().length;
        multiPlayAction.enabled = n > 0;
    }
    
    function itemType(data, indexPath)
    {
        if (helper.showTranslation) {
            return showImages ? "imageTrans" : "trans";
        } else {
            return showImages ? "image" : "text";
        }
    }

    multiSelectHandler
    {
        onActiveChanged: {
            if (active) {
                tutorial.execActionBar("selectRangeCopy", qsTr("Use the '%1' action if you want to copy the ayats to the clipboard so you can later paste it somewhere.").arg(multiCopy.title), undefined, true);
                tutorial.execActionBar("selectRangePlay", qsTr("Use the '%1' action if you want to play the recitation of the selected ayats.").arg(multiPlayAction.title), "l", true);
                tutorial.execActionBar("selectRangeShare", qsTr("Use the '%1' action if you want to share the ayats with one of your contacts or somewhere else.").arg(multiShare.title), "r", true);
            }
        }
        
        actions: [
            ActionItem
            {
                id: multiPlayAction
                enabled: false
                title: qsTr("Play") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_play.png"

                onTriggered: {
                    console.log("UserEvent: MultiPlay");
                    var selectedIndices = listView.selectionList();
                    var first = selectedIndices[0][0];
                    var last = selectedIndices[selectedIndices.length-1][0];

                    play(first, last);
                    reporter.record( "MultiPlay", verseModel.data(first).surah_id+":"+verseModel.data(first).verse_id+"-"+verseModel.data(last).surah_id+":"+verseModel.data(last).verse_id );
                }
            },
            
            ActionItem
            {
                id: multiCopy
                enabled: multiPlayAction.enabled
                title: qsTr("Copy") + Retranslate.onLanguageChanged
                imageSource: "images/common/ic_copy.png"
                
                onTriggered: {
                    console.log("UserEvent: MultiCopy");
                    persist.copyToClipboard( offloader.textualizeAyats(verseModel, selectionList(), ctb.text, helper.showTranslation) );
                    
                    reporter.record("MultiCopy");
                }
            },
            
            InvokeActionItem
            {
                id: multiShare
                enabled: multiPlayAction.enabled
                imageSource: "images/menu/ic_share.png"
                title: qsTr("Share") + Retranslate.onLanguageChanged
                
                query {
                    mimeType: "text/plain"
                    invokeActionId: "bb.action.SHARE"
                }
                
                onTriggered: {
                    console.log("UserEvent: MultiShare");
                    data = persist.convertToUtf8( offloader.textualizeAyats(verseModel, selectionList(), ctb.text, helper.showTranslation) );
                    
                    reporter.record("MultiShare");
                }
            }
        ]

        status: qsTr("None selected") + Retranslate.onLanguageChanged
    }
    
    function clearPrevious()
    {
        if (previousPlayedIndex >= 0)
        {
            var data = verseModel.value(previousPlayedIndex);
            
            if (data) {
                data.playing = false;
                verseModel.replace(previousPlayedIndex, data);
            }
        }
    }
    
    function onMetaDataChanged(metaData)
    {
        var index = recitation.extractIndex(metaData);
        
        if (previousPlayedIndex >= 0) {
            clearPrevious();
        }
        
        if (index == -1) {
            return;
        }
        
        var target = index;
        var data = dataModel.value(target);
        
        data["playing"] = true;
        verseModel.replace(target, data);
        
        if (follow) {
            listView.scrollToItem([target], ScrollAnimation.None);
        }
        
        previousPlayedIndex = index;
    }
    
    onCreationCompleted: {
        persist.registerForSetting(listView, "follow");
        persist.registerForSetting(listView, "overlayAyatImages");
        persist.registerForSetting(listView, "disableSpacing");
        player.metaDataChanged.connect(onMetaDataChanged);
        player.playbackCompleted.connect(clearPrevious);

        if (showImages) {
            tutorial.exec("overlayScroll", qsTr("Some ayats may be larger than your screen width. You need to scroll to the left to see the full ayat!"), HorizontalAlignment.Center, VerticalAlignment.Center, 0, 0, 0, 0, undefined, "r");
        } else {
            tutorial.execCentered("zoom", qsTr("Do a pinch gesture on the arabic text to increase or decrease the size of the font!"), "images/common/pinch.png");
            tutorial.exec("peekGesture", qsTr("To dismiss this page, you can do a peek gesture by swiping to the right from the left-corner."), HorizontalAlignment.Left, VerticalAlignment.Center, 0, 0, 0, 0, undefined, "r");
        }

        if (helper.showTranslation) {
            tutorial.exec("surahPageZoomTranslation", qsTr("Do a pinch gesture on the translation text to increase or decrease the size of the font!"), HorizontalAlignment.Center, VerticalAlignment.Center, 0, 0, 0, tutorial.du(12), "images/common/pinch.png");
        }
        
        tutorial.execActionBar( "repeat", qsTr("Tap on the repeat action at the bottom to enable or disable repeating the recitation in a loop once it finishes."), "l" );
        tutorial.execActionBar( "playAll", qsTr("Tap on the Play All button to play a recitation of all the verses on the screen.") );
        tutorial.execActionBar( "follow", qsTr("Use the follow button at the center of the left/right buttons if you want to follow the verses automatically as they are being recited."), "r" );
        tutorial.exec( "pressHoldVerse", qsTr("Tap on any verse to see more details about it.\n\nPress-and-hold on a verse to be able to play specific verses, or share them with others."), HorizontalAlignment.Center, VerticalAlignment.Center );
        tutorial.execActionBar( "backButton", qsTr("Tap on the Back button to return to the previous page."), "b" );
    }

    function onSettingChanged(newValue, key)
    {
        if (key == "follow") {
            follow = newValue == 1;
        } else if (key == "overlayAyatImages") {
            showImages = newValue > 0;
        } else if (key == "disableSpacing") {
            disableSpacing = newValue == 1;
        }
    }
    
    function memorize(from)
    {
        if (previousPlayedIndex >= 0) {
            clearPrevious();
        }
        
        previousPlayedIndex = -1;
        var end = Math.min( from+8, dataModel.size() );
        
        recitation.memorize(verseModel, from, end);
        reporter.record("Memorize", verseModel.value(from).surah_id+":"+verseModel.value(from).verse_id);
    }
    
    function onDataLoaded(id, data)
    {
        if (id == QueryId.SaveLastProgress) {
            persist.showToast( qsTr("Successfully set bookmark!"), "images/menu/ic_bookmark_add.png" );
            global.lastPositionUpdated();
        }
    }
    
    function setBookmark(ListItemData) {
        bookmarkHelper.saveLastProgress(listView, ListItemData.surah_id, ListItemData.verse_id);
        reporter.record("SetBookmark", ListItemData.surah_id+":"+ListItemData.verse_id);
    }
    
    function refresh()
    {
        for (var j = verseModel.size()-1; j >= 0; j--) {
            verseModel.replace( j, verseModel.value(j) );
        }
    }

    listItemComponents: [
        ListItemComponent
        {
            type: "image"
            AyatImageListItem {}
        },
        
        ListItemComponent
        {
            type: "imageTrans"
            AyatImageTranslationListItem {}
        },
        
        ListItemComponent
        {
            type: "trans"
            AyatTranslationListItem {}
        },
        
        ListItemComponent
        {
            type: "text"
            AyatListItem {}
        }
    ]
    
    attachedObjects: [
        RangeSelector {
            id: rangeSelector
            itemName: qsTr("ayahs")
        },
        
        ImagePaintDefinition
        {
            id: activeDef
            imageSource: "images/list_item_pressed.amd"
        },
        
        Timer {
            id: timer
            interval: 150
            running: false
            repeat: false
            
            onTriggered: {
                scrolled = false;
            }
        }
    ]
}