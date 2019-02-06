TEMPLATE = aux

scripts.files = \
    pm-apply \
    pm-unapply
scripts.path = /usr/libexec

constants.files = pm-constants
constants.path = /etc

INSTALLS += scripts constants
