TEMPLATE      = subdirs
SUBDIRS      += comapp \
                simple \
                wrapper

qtHaveModule(quickcontrols2):SUBDIRS += simpleqml
