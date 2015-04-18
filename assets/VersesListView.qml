import QtQuick 1.0
import bb.cascades 1.3
import com.canadainc.data 1.0

ListView
{
    id: listView
    property alias theDataModel: verseModel
    property alias activeDefinition: activeDef
    property int chapterNumber
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
    
    onScrolledChanged: {
        if (scrolled) {
            timer.restart();
        }
    }
    
    leadingVisual: BismillahControl {
        delegateActive: chapterNumber > 1 && chapterNumber != 9
    }
    
    function play(from, to)
    {
        clearPrevious();
        previousPlayedIndex = -1;
        recitation.downloadAndPlayAll(verseModel, from, to);
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
                tutorial.exec("selectRangeCopy", qsTr("Use the Copy action if you want to copy the ayats to the clipboard so you can later paste it somewhere."), HorizontalAlignment.Center, VerticalAlignment.Bottom);
                tutorial.exec("selectRangePlay", qsTr("Use the Play action if you want to play the recitation of the selected ayats."), HorizontalAlignment.Left, VerticalAlignment.Bottom, ui.du(22));
                tutorial.exec("selectRangeShare", qsTr("Use the Share action if you want to share the ayats with one of your contacts or somewhere else."), HorizontalAlignment.Right, VerticalAlignment.Bottom, 0, ui.du(22));
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
                }
            },
            
            ActionItem
            {
                id: multiCopy
                enabled: multiPlayAction.enabled
                title: qsTr("Copy") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_copy.png"
                
                onTriggered: {
                    console.log("UserEvent: MultiCopy");
                    persist.copyToClipboard( offloader.textualizeAyats(verseModel, selectionList(), ctb.text, helper.showTranslation) );
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
                }
            }
        ]

        status: qsTr("None selected") + Retranslate.onLanguageChanged
    }
    
    function clearPrevious()
    {
        var data = verseModel.value(previousPlayedIndex);
        data.playing = false;
        verseModel.replace(previousPlayedIndex, data);
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
        persist.settingChanged.connect(onSettingChanged);
        player.metaDataChanged.connect(onMetaDataChanged);
        player.playbackCompleted.connect(clearPrevious);
        
        onSettingChanged("follow");
        onSettingChanged("overlayAyatImages");

        if (showImages) {
            tutorial.exec("overlayScroll", qsTr("Some ayats may be larger than your screen width. You need to scroll to the left to see the full ayat!"), HorizontalAlignment.Center, VerticalAlignment.Center, 0, 0, 0, 0, undefined, "r");
        } else {
            tutorial.execCentered("zoom", qsTr("Do a pinch gesture on the arabic text to increase or decrease the size of the font!"), "images/tutorial/pinch.png");
            tutorial.exec("peekGesture", qsTr("To dismiss this page, you can do a peek gesture by swiping to the right from the left-corner."), HorizontalAlignment.Left, VerticalAlignment.Center, 0, 0, 0, 0, undefined, "r");
        }

        if (helper.showTranslation) {
            tutorial.exec("surahPageZoomTranslation", qsTr("Do a pinch gesture on the translation text to increase or decrease the size of the font!"), HorizontalAlignment.Center, VerticalAlignment.Center, 0, 0, 0, ui.du(12), "images/tutorial/pinch.png");
        }
        
        tutorial.execActionBar( "repeat", qsTr("Tap on the repeat action at the bottom to enable or disable repeating the recitation in a loop once it finishes."), "r" );
        tutorial.execActionBar( "playAll", qsTr("Tap on the Play All button to play a recitation of all the verses on the screen.") );
        tutorial.exec( "pressHoldVerse", qsTr("Tap on any verse to see more details about it.\n\nPress-and-hold on a verse to be able to play specific verses, or share them with others."), HorizontalAlignment.Center, VerticalAlignment.Center );
        tutorial.exec( "backButton", qsTr("Tap on the Back key to return to the previous page."), HorizontalAlignment.Left, VerticalAlignment.Bottom );
    }

    function onSettingChanged(key)
    {
        if (key == "follow") {
            follow = persist.getValueFor("follow") == 1;
        } else if (key == "overlayAyatImages") {
            showImages = persist.getValueFor("overlayAyatImages") == 1;
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