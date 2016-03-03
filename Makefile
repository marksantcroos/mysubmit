all: main mw

clean:
	rm -rf main main.dSYM mw mw.dSYM

CFLAGS = `PKG_CONFIG_PATH=\`ompi_info --path libdir --parsable|cut -d: -f3\`/pkgconfig pkg-config --define-variable=pkgincludedir=\`ompi_info --path pkgincludedir --parsable|cut -d: -f3\` --cflags orte opal`
LDFLAGS = `PKG_CONFIG_PATH=\`ompi_info --path libdir --parsable|cut -d: -f3\`/pkgconfig pkg-config --define-variable=pkgincludedir=\`ompi_info --path pkgincludedir --parsable|cut -d: -f3\` --libs orte opal`

main: main.c
	gcc -Wall -g -o main main.c $(CFLAGS) $(LDFLAGS)

mw: mw.c
	gcc -Wall -g -o mw mw.c $(CFLAGS) $(LDFLAGS)
