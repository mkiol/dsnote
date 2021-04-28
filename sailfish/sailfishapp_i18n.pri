#
# sailfishapp_i18n.prf: Qt Feature file for Translated Sailfish apps
# Usage: CONFIG += sailfishapp_i18n
#
# Copyright (c) 2014 Jolla Ltd.
# Contact: Thomas Perl <thomas.perl@jolla.com>
# All rights reserved.
#
# This file is part of libsailfishapp
#
# You may use this file under the terms of BSD license as follows:
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Jolla Ltd nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

TRANSLATIONS_OUT_DIR_NAME = "translations"
HAVE_TRANSLATIONS = 0

# Translation source directories
TRANSLATION_SOURCE_CANDIDATES = $$TRANSLATION_SOURCE_DIRS
for(dir, TRANSLATION_SOURCE_CANDIDATES) {
    exists($$dir) {
        TRANSLATION_SOURCES += $$clean_path($$dir)
    }
}

# prefix all TRANSLATIONS with the src dir
# the qm files are generated from the ts files copied to out dir
for(t, TRANSLATIONS) {
    TRANSLATIONS_IN  += $$clean_path($$t)
    TRANSLATIONS_OUT += $$clean_path($${OUT_PWD}/$${TRANSLATIONS_OUT_DIR_NAME}/$$basename(t))
    HAVE_TRANSLATIONS = 1
}

qm.files = $$replace(TRANSLATIONS_OUT, \.ts, .qm)
qm.path = /usr/share/$${TARGET}/translations
qm.CONFIG += no_check_exist

# update the ts files in the src dir and then copy them to the out dir
sailfishapp_i18n_include_obsolete {
    qm.commands += lupdate $${TRANSLATION_SOURCES} -ts $$TRANSLATIONS_IN && \
        mkdir -p translations && \
        [ \"$${OUT_PWD}\" != \"$${_PRO_FILE_PWD_}\" -a $$HAVE_TRANSLATIONS -eq 1 ] && \
        cp -af $${TRANSLATIONS_IN} $${OUT_PWD}/translations || :
} else {
    qm.commands += lupdate -noobsolete $${TRANSLATION_SOURCES} -ts $$TRANSLATIONS_IN && \
        mkdir -p translations && \
        [ \"$${OUT_PWD}\" != \"$${_PRO_FILE_PWD_}\" -a $$HAVE_TRANSLATIONS -eq 1 ] && \
        cp -af $${TRANSLATIONS_IN} $${OUT_PWD}/translations || :
}

sailfishapp_i18n_unfinished {
    TRANSLATE_UNFINISHED =
} else {
    TRANSLATE_UNFINISHED = -nounfinished
}

# create the qm files
sailfishapp_i18n_idbased {
    qm.commands += ; [ $$HAVE_TRANSLATIONS -eq 1 ] && lrelease -idbased $${TRANSLATE_UNFINISHED} $${TRANSLATIONS_OUT} || :
} else {
    qm.commands += ; [ $$HAVE_TRANSLATIONS -eq 1 ] && lrelease $${TRANSLATE_UNFINISHED} $${TRANSLATIONS_OUT} || :
}

INSTALLS += qm
