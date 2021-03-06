import bb.cascades 1.2
import com.canadainc.data 1.0

ResizableContainer
{
    headerTitle: qsTr("Explanations") + Retranslate.onLanguageChanged
    
    onCreationCompleted: {
        helper.fetchAllTafsirForAyat(tafsirList, root.surahId, root.verseId);
    }
    
    Container
    {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Top
        layout: DockLayout {}
        maxHeight: screenHeight*ratio
        
        Container
        {
            background: Color.Black
            opacity: 0
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            
            animations: [
                FadeTransition {
                    id: ft
                    fromOpacity: 0
                    toOpacity: 0.45
                    easingCurve: StockCurve.CubicOut
                    duration: 1200
                    
                    onEnded: {
                        tutorial.execBelowTitleBar( "tafsirPicking", qsTr("These are the explanations for this verse from the various people of knowledge. As more tafsir is available the app will automatically try to fetch them and update its database in the future with your permission. Tap on any one of them to open it and read it."), tutorial.du(20) );
                        tutorial.execBelowTitleBar( "tafsirShortcut", qsTr("Press-and-hold on any of the tafsir and choose 'Add Shortcut' to pin it to your homescreen."), tutorial.du(15) );
                    }
                }
            ]
        }
        
        ListView
        {
            id: tafsirList
            scrollRole: ScrollRole.Main
            
            function onDataLoaded(id, data)
            {
                if (id == QueryId.FetchTafsirForAyat)
                {
                    adm.append(data);
                    offloader.decorateTafsir(adm);
                    
                    if (data.length == 1) {
                        showExplanation(data[0].id);
                    }
                    
                    headerSubtitle = data.length;
                    ft.play();
                }
            }

            function onFinished(name, tafsirId)
            {
                if (name.length > 0)
                {
                    offloader.addToHomeScreen(tafsirId, name);
                    reporter.record("AddTafsirHome", tafsirId.toString());
                }
            }
            
            function addToHomeScreen(ListItemData) {
                persist.showPrompt( tafsirList, qsTr("Enter name"), qsTr("You can use this to quickly recognize this tafsir on your home screen."), ListItemData.title, qsTr("Shortcut name..."), 15, "onFinished", ListItemData.id );
            }
            
            onTriggered: {
                console.log("UserEvent: TafsirTriggered");
                showExplanation( dataModel.data(indexPath).id );
            }
            
            dataModel: ArrayDataModel {
                id: adm
            }
            
            listItemComponents: [
                ListItemComponent
                {
                    StandardListItem
                    {
                        id: rootItem
                        title: ListItemData.author
                        description: ListItemData.heading && ListItemData.heading.length > 0 ? ListItemData.heading : ListItemData.title
                        imageSource: ListItemData.imageSource
                        translationY: -150
                        
                        animations: [
                            TranslateTransition
                            {
                                id: tt
                                fromY: -150
                                toY: 0
                                delay: Math.min( 1250, rootItem.ListItem.indexPath[0]*150 )
                                easingCurve: StockCurve.CircularOut
                                duration: 750
                            }
                        ]
                        
                        ListItem.onInitializedChanged: {
                            if (initialized) {
                                tt.play();
                            }
                        }
                        
                        contextActions: [
                            ActionSet
                            {
                                title: rootItem.title
                                subtitle: rootItem.description
                                
                                ActionItem
                                {
                                    title: qsTr("Add Shortcut") + Retranslate.onLanguageChanged
                                    imageSource: "images/menu/ic_home.png"
                                    
                                    onTriggered: {
                                        console.log("UserEvent: AddTafsirShortcutFromPicker");
                                        rootItem.ListItem.view.addToHomeScreen(rootItem.ListItem.data); 
                                    }
                                }
                            }
                        ]
                    }
                }
            ]
        }
    }
}