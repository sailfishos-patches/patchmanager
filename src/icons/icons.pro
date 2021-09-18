TEMPLATE = aux
# Configures svg to png
THEMENAME=sailfish-default

load(sailfish-svg2png-sizes)

theme.path = /usr/share/themes/$${THEMENAME}/
themeDirectory.path = /usr/share/themes/$${THEMENAME}/meegotouch
themeDirectory.files += $${OUT_PWD}/icons

for(profile, SAILFISH_SVG2PNG.profiles) {
    # a work-around for installing directories that do not exist yet
    system(mkdir -p $${OUT_PWD}/z$${profile})

    exists( $${OUT_PWD}/symlinks ): svg2png.commands += cp -r symlinks/icons z$${profile} &&

    svg2png.commands += /usr/bin/sailfish_svg2png \
        -z $$eval(SAILFISH_SVG2PNG.scales.$${profile}) \
        $$eval(SAILFISH_SVG2PNG.extra.$${profile}) \
         $${_PRO_FILE_PWD_}/svgs \
         z$${profile}/icons &&

    svg2png.files += $${OUT_PWD}/z$${profile}
}

svg2png.commands += true
svg2png.path = $${themeDirectory.path}

INSTALLS += svg2png theme themeDirectory
