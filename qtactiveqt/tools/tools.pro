TEMPLATE = subdirs

CONFIG  += ordered

SUBDIRS = dumpdoc \
          dumpcpp \
          testcon

qtNomakeTools( \
    testcon \
)
