QT = core
TEMPLATE = subdirs
SUBDIRS =\
    plugin\
    plugin.2\
    plugin.2.1\
    pluginWrongCase\
    pluginWithQmlFile\
    pluginMixed\
    pluginVersion\
    nestedPlugin\
    strictModule\
    strictModule.2\
    nonstrictModule\
    preemptiveModule\
    preemptedStrictModule\
    invalidNamespaceModule\
    invalidFirstCommandModule\
    protectedModule\
    plugin/childplugin\
    plugin.2/childplugin\
    plugin.2.1/childplugin\
    plugin.2.2\
    moduleWithQmlSingleton

tst_qqmlmoduleplugin_pro.depends += plugin
SUBDIRS += tst_qqmlmoduleplugin.pro

QT += core-private gui-private qml-private
