import bb.cascades 1.0

QtObject
{
    property string itemName
    property MultiSelectActionItem msai: MultiSelectActionItem
    {
        title: qsTr("Select Range") + Retranslate.onLanguageChanged
        imageSource: "images/menu/ic_range.png"
    }

    function onSelectionChanged()
    {
        var all = parent.selectionList();
        var n = all.length;
        var first = all[0][0];
        var last = all[n-1][0];

        for (var i = first; i < last; i ++) {
            parent.select([i,0], true);
        }
        
        parent.multiSelectHandler.status = qsTr("%n %1 selected", "", last-first+1).arg(itemName) + Retranslate.onLanguageChanged;
        
        var multiActions = parent.multiSelectHandler.actions;
        
        for (var i = multiActions.length-1; i >= 0; i--) {
            multiActions[i].enabled = n > 0;
        }
    }
    
    onCreationCompleted: {
        parent.selectionChanged.connect(onSelectionChanged);
        parent.multiSelectAction = msai;
    }
}