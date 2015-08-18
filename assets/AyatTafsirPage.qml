import bb.cascades 1.0

Page
{
    id: root
    property alias suitePageId: parser.suitePageId
    actionBarAutoHideBehavior: ActionBarAutoHideBehavior.HideOnScroll
    
    onCreationCompleted: {
        content = parser.mainContent;
    }
    
    onSuitePageIdChanged: {
        parser.scalerAnim.play();
    }
    
    attachedObjects: [
        AyatTafsirParser {
            id: parser
            
            onNotFound: {
                var params = {'language': helper.translation, 'tafsir': helper.tafsirName, 'translation': helper.translationName};
                helper.updateCheckNeeded(params);
            }
        }
    ]
}