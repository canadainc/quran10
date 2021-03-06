import bb.cascades 1.2

Container
{
    id: itemRoot
    property bool peek: ListItem.view.secretPeek != undefined ? ListItem.view.secretPeek : false
    property bool playing: ListItemData.playing ? ListItemData.playing : false
    property alias actionSetSubtitle: actionSet.subtitle
    horizontalAlignment: HorizontalAlignment.Fill
    
    function updateState(selected)
    {
        if (playing) {
            background = Color.create("#ffff8c00");
        } else if (ListItem.selected) {
            background = Color.DarkGreen;
        } else if (ListItem.active) {
            background = ListItem.view.activeDefinition.imagePaint;
        } else {
            background = undefined;
        }
    }
    
    onCreationCompleted: {
        ListItem.activationChanged.connect(updateState);
        ListItem.selectionChanged.connect(updateState);
        playingChanged.connect(updateState);
        updateState();
    }
    
    onPeekChanged: {
        if (peek) {
            showAnim.play();
        }
    }
    
    opacity: 0
    animations: [
        FadeTransition
        {
            id: showAnim
            fromOpacity: 0
            toOpacity: 1
            duration: Math.max( 200, Math.min( itemRoot.ListItem.indexPath[0]*300, 750 ) );
            easingCurve: StockCurve.QuadraticIn
        }
    ]
    
    ListItem.onInitializedChanged: {
        if (initialized) {
            showAnim.play();
        }
    }
    
    Container
    {
        id: headerRoot
        horizontalAlignment: HorizontalAlignment.Fill
        background: global.headerBackground.imagePaint
        topPadding: 5
        bottomPadding: 5
        leftPadding: 5
        
        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }
        
        Label {
            id: headerLabel
            text: "%1:%2".arg(ListItemData.surah_id).arg(ListItemData.verse_id)
            horizontalAlignment: HorizontalAlignment.Fill
            textStyle.fontSize: FontSize.XXSmall
            textStyle.color: Color.White
            textStyle.fontWeight: FontWeight.Bold
            textStyle.textAlign: TextAlign.Center
            
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1
            }
        }
    }
    
    contextMenuHandler: [
        ContextMenuHandler
        {
            id: cmh
            
            onPopulating: {
                if (!itemRoot.ListItem.view.showContextMenu) {
                    event.abort();
                } else {
                    var all = itemRoot.ListItem.view.selectionList();
                    
                    if (all && all.length > 0 && all[0] != itemRoot.ListItem.indexPath) {
                        itemRoot.ListItem.view.select(all[0], false);
                    }
                }
            }

            onVisualStateChanged: {
                if (cmh.visualState == ContextMenuVisualState.VisibleCompact)
                {
                    tutorial.execOverFlow("memorize", qsTr("%1: This mode begins the playback of the current verse followed by up to the next 7 verses 20 times each to help you memorize it."), memorize);
                    tutorial.execOverFlow("playFromHere", qsTr("%1: This begins playback of the recitation starting from this verse."), playFromHere);
                    tutorial.execOverFlow("setBookmark", qsTr("You can use the '%1' action to place a bookmark on this verse so you can resume your reading the next time right to this verse quickly."), setBookmark);
                    tutorial.execOverFlow("selectRangeOption", qsTr("You can use the '%1' action to only play recitations for specific ayat, or copy/share them to your contacts."), itemRoot.ListItem.view.multiSelectAction);
                }
            }
        }
    ]
    
    contextActions: [
        ActionSet
        {
            id: actionSet
            title: ListItemData.arabic
            subtitle: headerLabel.text
            
            ActionItem
            {
                id: memorize
                title: qsTr("Memorize") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_memorize.png"
                
                onTriggered: {
                    console.log("UserEvent: MemorizeAyat");
                    itemRoot.ListItem.view.memorize( itemRoot.ListItem.indexPath[0] );
                }
            }
            
            ActionItem
            {
                id: playFromHere
                
                title: qsTr("Play From Here") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_play.png"
                
                onTriggered: {
                    console.log("UserEvent: PlayFromHere");
                    itemRoot.ListItem.view.play(itemRoot.ListItem.indexPath[0], -1);
                }
            }

            ActionItem
            {
                id: setBookmark
                title: qsTr("Set Bookmark") + Retranslate.onLanguageChanged
                imageSource: "images/menu/ic_bookmark_add.png"

                onTriggered: {
                    console.log("UserEvent: SetBookmark");
                    itemRoot.ListItem.view.setBookmark(itemRoot.ListItem.data);
                }
            }
        }
    ]
}