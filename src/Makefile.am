#SUBDIRS = Network

bin_PROGRAMS = RealTimeBattle
include_HEADERS = Messagetypes.h

rtbheaderdir = $(RTB_DIR)
rtbheader_DATA = Messagetypes.h

if ENABLE_PROF
PROFFLAGS = -pg
endif

if NO_GRAPHICS
GRAPHICS_O = 
GRAPHFLAGS += -DNO_GRAPHICS 
else
GRAPHICS_O = \
	Gui.o ArenaWindow.o \
	ControlWindow.o Dialog.o \
	MessageWindow.o \
	OptionsWindow.o \
	ScoreWindow.o StatisticsWindow.o \
	StartTournamentWindow.o Structs.o
endif

INCLUDES = $(GTK_CFLAGS) -I$(top_srcdir)/intl $(PROFFLAGS) $(GRAPHFLAGS)

if NO_GETOPT_LONG
GETOPT_O = getopt.o
else
GETOPT_O =
endif

if ENABLE_CCMALLOC
CCMALLOC_O = /usr/lib/ccmalloc.o -ldl
else
CCMALLOC_O =
endif

RealTimeBattle_LDADD = $(GETOPT_O) $(GRAPHICS_O) $(INTLLIBS) $(CCMALLOC_O) $(GTK_LIBS) 
RealTimeBattle_DEPENDENCIES = $(GETOPT_O) $(GRAPHICS_O) $(INTLOBJS)
RealTimeBattle_SOURCES = \
	ArenaBase.cc ArenaBase.h \
	ArenaController.cc ArenaController.h \
	ArenaRealTime.cc ArenaRealTime.h \
	ArenaReplay.h ArenaReplay.cc \
	Extras.cc Extras.h \
	IntlDefs.h \
	GeometricalObjects.cc GeometricalObjects.h \
	List.cc List.h \
	Messagetypes.h \
	MovingObject.h \
	Options.cc Options.h \
	RealTimeBattle.cc \
	Shape.cc Shape.h \
	String.cc String.h \
	Various.cc Various.h \
	Vector2D.cc Vector2D.h \
	Robot.cc Robot.h \
	Shot.cc Shot.h \
	Structs.h \
	Timer.cc Timer.h \
	Wall.h 


EXTRA_RealTimeBattle_SOURCES = \
	ArenaWindow.cc ArenaWindow.h \
	ControlWindow.cc ControlWindow.h \
	Dialog.cc Dialog.h getopt.c getopt.h \
	Gui.cc Gui.h \
	MessageWindow.cc MessageWindow.h \
	OptionsWindow.cc OptionsWindow.h \
	ScoreWindow.cc ScoreWindow.h \
	StartTournamentWindow.cc StartTournamentWindow.h \
	StatisticsWindow.cc StatisticsWindow.h \
	Structs.cc

