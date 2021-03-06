import bb.cascades 1.2
import com.canadainc.data 1.0

Page
{
    id: mainPage
    signal picked(int chapter, int verse)
    signal juzPicked(int juzId)
    actionBarAutoHideBehavior: ActionBarAutoHideBehavior.HideOnScroll
    property alias pickerList: listView
    property bool showJuz: false
    property alias sortValue: sortOrder.selectedValue
    property alias normalMode: normal.text
    property alias juzMode: juzOption.text
    property bool focusOnSearchBar: false
    property alias titleBarSpace: button.controls

    titleBar: TitleBar
    {
        kind: TitleBarKind.FreeForm
        scrollBehavior: TitleBarScrollBehavior.NonSticky
        kindProperties: FreeFormTitleBarKindProperties
        {
            Container
            {
                id: titleBar
                background: titleBack.imagePaint
                rightPadding: 50
                layout: DockLayout {}
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Top
                
                ImageView
                {
                    imageSource: "images/title/logo.png"
                    loadEffect: ImageViewLoadEffect.FadeZoom
                    horizontalAlignment: HorizontalAlignment.Right
                    verticalAlignment: VerticalAlignment.Center
                    opacity: 0
                    
                    animations: [
                        FadeTransition {
                            id: fadeInLogo
                            easingCurve: StockCurve.CubicIn
                            fromOpacity: 0
                            toOpacity: 1
                            duration: 1000
                            
                            onEnded: {
                                if (showJuz) {
                                    tm.process();
                                }
                            }
                        }
                    ]
                }
                
                Container
                {
                    id: button
                    leftPadding: 15;
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layout: DockLayout {}
                }
                
                attachedObjects: [
                    ImagePaintDefinition {
                        id: titleBack
                        imageSource: "images/title/title_bg.png"
                    }
                ]
            }
        }
    }
    
    onPeekedAtChanged: {
        listView.secretPeek = peekedAt;
    }
    
    Container
    {
        background: back.imagePaint
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: DockLayout {}
        
        Container
        {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            
            Container
            {
                layout: DockLayout {}
                background: Color.Black
                horizontalAlignment: HorizontalAlignment.Fill
                visible: showJuz
                
                PersistDropDown
                {
                    id: sortOrder
                    title: qsTr("Display Options") + Retranslate.onLanguageChanged
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 0; bottomMargin: 0
                    
                    Option {
                        id: alphabet
                        text: qsTr("Alphabetical Order") + Retranslate.onLanguageChanged
                        description: qsTr("Sorted by surah name") + Retranslate.onLanguageChanged
                        imageSource: "images/dropdown/sort_alphabet.png"
                        value: "name"
                    }
                    
                    Option {
                        id: juzOption
                        text: qsTr("Juz") + Retranslate.onLanguageChanged
                        description: qsTr("The surahs will be displayed separated in juz") + Retranslate.onLanguageChanged
                        imageSource: "images/dropdown/sort_juz.png"
                        value: "juz"
                    }
                    
                    Option {
                        id: normal
                        text: qsTr("Normal") + Retranslate.onLanguageChanged
                        description: qsTr("The surahs will be displayed in the standard order") + Retranslate.onLanguageChanged
                        imageSource: "images/dropdown/sort_normal.png"
                        value: "normal"
                    }
                    
                    Option {
                        id: recent
                        text: qsTr("Revelation Order") + Retranslate.onLanguageChanged
                        description: qsTr("Display chapters in the order they were revealed") + Retranslate.onLanguageChanged
                        imageSource: "images/dropdown/sort_revelation.png"
                        value: "revelation_order"
                    }
                    
                    onExpandedChanged: {
                        if (expanded)
                        {
                            tutorial.execBelowTitleBar( "alphabetical", qsTr("To arrange the surahs in ascending order relative to their name use the '%1' option.").arg(alphabet.text) );
                            tutorial.execBelowTitleBar( "juz", qsTr("To categorize the surahs into their respective Juz, use the '%1' option.").arg(juzOption.text), tutorial.du(8) );
                            tutorial.execBelowTitleBar( "normal", qsTr("To arrange the surahs in the order they appear in the Mushaf, use the '%1' option.").arg(normal.text), tutorial.du(16) );
                            tutorial.execBelowTitleBar( "recent", qsTr("To arrange the surahs in their original order of revelation (ascending), use the '%1' option.").arg(recent.text), tutorial.du(24) );
                        }
                    }
                    
                    onValueChanged: {
                        if (diff) {
                            reporter.record("DisplayOption", sortOrder.selectedValue);
                        }
                    }
                    
                    onSelectedOptionChanged: {
                        textField.lastSearch = textField.lastSearch+"X";
                        textField.textChanging(textField.text);
                    }
                }
            }
            
            TextField
            {
                id: textField
                hintText: qsTr("Search surah name or number (ie: '2' for Surah Al-Baqara)...") + Retranslate.onLanguageChanged
                bottomMargin: 0
                horizontalAlignment: HorizontalAlignment.Fill
                topMargin: 0;
                property string lastSearch
                
                onTextChanging: {
                    var textValue = text.trim();
                    
                    if ( textValue != lastSearch || gdm.isEmpty() )
                    {
                        if (textValue.length == 0 && sortOrder.selectedOption == juzOption) {
                            helper.fetchAllChapters(listView);
                        } else if ( textValue.match(/^\d{1,3}:\d{1,3}$/) || textValue.match(/^\d{1,3}:{0,1}$/) ) {
                            var tokens = textValue.split(":");
                            var surah = parseInt(tokens[0]);
                            helper.fetchChapter(listView, surah);
                        } else {
                            helper.fetchChapters(listView, textValue);
                        }
                        
                        lastSearch = textValue;
                        busy.delegateActive = true;
                    }
                }
                
                input {
                    submitKey: SubmitKey.Submit
                    
                    onSubmitted: {
                        var textValue = text.trim();
                        
                        if ( textValue.match(/^\d{1,3}:\d{1,3}$/) || textValue.match(/^\d{1,3}:{0,1}$/) )
                        {
                            var tokens = textValue.split(":");
                            var surah = parseInt(tokens[0]);
                            
                            if (tokens.length > 0) {
                                var verse = parseInt(tokens[1]);
                                reporter.record("SurahVerseShortcut", surah+":"+verse);
                                picked(surah, verse);
                            } else {
                                reporter.record("SurahVerseShortcut", surah+":0");
                                picked(surah, 0);
                            }
                        }
                    }
                }
                
                animations: [
                    TranslateTransition {
                        id: translate
                        fromX: 1000
                        duration: 500
                        
                        onEnded: {
                            if (deviceUtils.isPhysicalKeyboardDevice || focusOnSearchBar) {
                                textField.requestFocus();
                            }
                            
                            textField.input["keyLayout"] = 7;
                            deviceUtils.attachTopBottomKeys(mainPage, listView);
                            tutorial.execBelowTitleBar("qPaneSurahPicker", qsTr("Search for surah names here. For example you can enter 'Baqara' or 'Fatiha'.\n\nYou can also enter surah numbers like '2' to go directly to Surah Baqara.\n\nYou can also enter '2:3' to go to Surah Baqara verse #3"), tutorial.du(8));
                            tutorial.execBelowTitleBar("displayStyle", qsTr("Tap here on the '%1' to change the order of the surahs or display Juz related information!").arg(sortOrder.title) );
                        }
                    }
                ]
            }
            
            ListView
            {
                id: listView
                objectName: "listView"
                property bool secretPeek: false
                scrollRole: ScrollRole.Main
                
                dataModel: GroupDataModel
                {
                    id: gdm
                    sortingKeys: ["surah_id"]
                    grouping: ItemGrouping.ByFullValue
                }
                
                listItemComponents: [
                    ListItemComponent {
                        type: "header"
                        
                        Header {
                            title: qsTr("Juz %1").arg(ListItemData)
                            subtitle: ListItem.view.dataModel.childCount(ListItem.indexPath)
                        }
                    },
                    
                    ListItemComponent
                    {
                        type: "item"
                        
                        StandardListItem
                        {
                            id: sli
                            property bool peek: ListItem.view.secretPeek
                            title: ListItemData.transliteration ? ListItemData.transliteration : ListItemData.name
                            description: ListItemData.transliteration ? ListItemData.name : qsTr("%n ayahs", "", ListItemData.verse_count)
                            status: ListItemData.verse_number ? ListItemData.verse_number : ListItemData.surah_id
                            imageSource: "images/ic_quran.png"
                            contextActions: ActionSet {}
                            
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
                                    easingCurve: StockCurve.SineOut
                                    fromOpacity: 0
                                    toOpacity: 1
                                    duration: Math.min( sli.ListItem.indexInSection*300, 750 );
                                }
                            ]
                            
                            ListItem.onInitializedChanged: {
                                if (initialized) {
                                    showAnim.play();
                                }
                            }
                        }
                    }
                ]
                
                onTriggered: {
                    var data = listView.dataModel.data(indexPath);
                    
                    if (data.surah_id) {
                        console.log("UserEvent: SurahTriggered");
                        picked(data.surah_id, data.verse_number ? data.verse_number : 0);
                        reporter.record("SurahTriggered", data.surah_id+":"+(data.verse_number ? data.verse_number : 0));
                    } else {
                        console.log("UserEvent: JuzTriggered");
                        juzPicked(data);
                        reporter.record("JuzTriggered", data.toString());
                    }
                }
                
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                
                function onDataLoaded(id, data)
                {
                    if (id == QueryId.FetchChapters)
                    {
                        gdm.grouping = ItemGrouping.None;
                        
                        if (sortOrder.selectedOption == alphabet) {
                            gdm.sortingKeys = [helper.showTranslation ? "transliteration" : "name"];
                        } else if (sortOrder.selectedOption == normal || sortOrder.selectedOption == null) {
                            gdm.sortingKeys = ["surah_id"];
                        } else if (sortOrder.selectedOption == recent) {
                            gdm.sortingKeys = ["revelation_order"];
                        }
                        
                    } else if (id == QueryId.FetchAllChapters) {
                        gdm.grouping = ItemGrouping.ByFullValue;
                        gdm.sortingKeys = ["juz_id", "surah_id", "verse_number"];
                        data = offloader.normalizeJuzs(data);
                        
                        tutorial.execBelowTitleBar("juzHeader", qsTr("Tap on any of the Juz headers to open that Juz, or tap on any of the surahs to open them individually."), tutorial.du(16));
                    }
                    
                    var total = gdm.size();
                    var n = data.length;
                    
                    if (total == 1 && n == 1)
                    {
                        var firstIndex = gdm.first();
                        
                        if ( gdm.data(firstIndex).surah_id != data[0].surah_id ) { // optimization
                            gdm.updateItem( gdm.first(), data[0] );
                        }
                    } else {
                        gdm.clear();
                        gdm.insertList(data);
                    }

                    noElements.delegateActive = gdm.isEmpty();
                    listView.visible = !noElements.delegateActive;
                    busy.delegateActive = false;
                }
            }
        }
        
        EmptyDelegate
        {
            id: noElements
            graphic: "images/placeholders/empty_chapters.png"
            labelText: qsTr("No chapters matched your search criteria. Please try a different search term.") + Retranslate.onLanguageChanged
            
            onImageTapped: {
                console.log("UserEvent: NoChaptersTapped");
                textField.requestFocus();
            }
        }
        
        ProgressControl
        {
            id: busy
            asset: "images/progress/loading_chapters.png"
        }
        
        PermissionToast
        {
            id: tm
            horizontalAlignment: HorizontalAlignment.Center
            
            function process()
            {
                var allMessages = [];
                var allIcons = [];
                
                if ( !persist.hasSharedFolderAccess() ) {
                    allMessages.push("Warning: It seems like the app does not have access to your Shared Folder. This permission is needed to download the recitation audio and the mushaf pages. If you leave this permission off, some features may not work properly.");
                    allIcons.push("images/toast/ic_no_shared_folder.png");
                }
                
                if (allMessages.length > 0)
                {
                    messages = allMessages;
                    icons = allIcons;
                    delegateActive = true;
                }
            }
        }
    }
    
    function cleanUp()
    {
        helper.textualChange.disconnect(onChanged);
        tutorial.tutorialFinished.disconnect(onTutorialFinished);
    }
    
    function ready()
    {
        sortOrder.key = "surahPickerOption";
        
        if ( !persist.contains("surahPickerOption") ) {
            normal.selected = true;
        }
        
        fadeInLogo.play();
        translate.play();
        
        helper.textualChange.connect(onChanged);
        tutorial.tutorialFinished.connect(onTutorialFinished);
    }
    
    function onChanged() {
        sortOrder.selectedOptionChanged(undefined);
    }
    
    function onTutorialFinished(key)
    {
        if (key == "displayStyle") {
            sortOrder.expanded = true;
        }
    }
    
    attachedObjects: [
        ImagePaintDefinition {
            id: back
            imageSource: "images/backgrounds/background.png"
        }
    ]
}
