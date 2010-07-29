# src/Makefile.inc

rel := src

DCL  += $(rel)/config.h
DIRS += $(rel)
DIST += $(rel)/Makefile.inc $(rel)/config.h.in $(rel)/config.tc

# ----------------------------------------------------------------------

$(rel)/%.o: $(rel)/%.c
	$(QP)echo "  CC     $@"
	$(QR)$(CC) -c -o $@ $(CFLAGS_DEFAULT) $<


# ----------------------------------------------------------------------
# mpegdemux

MPEGDEMUX_BAS := \
	buffer \
	getopt \
	message \
	mpegdemux \
	mpeg_demux \
	mpeg_list \
	mpeg_parse \
	mpeg_remux \
	mpeg_scan

MPEGDEMUX_SRC  := $(foreach f,$(MPEGDEMUX_BAS),$(rel)/$(f).c)
MPEGDEMUX_OBJ  := $(foreach f,$(MPEGDEMUX_BAS),$(rel)/$(f).o)
MPEGDEMUX_HDR  := $(foreach f,$(MPEGDEMUX_BAS),$(rel)/$(f).h)
MPEGDEMUX_HDR  += $(rel)/mpeg_ints.h
MPEGDEMUX_MAN1 := $(rel)/mpegdemux.1
MPEGDEMUX_BIN  := $(rel)/mpegdemux$(EXEEXT)
MPEGDEMUX_SDP  := $(MPEGDEMUX_HDR) $(rel)/config.h
MPEGDEMUX_BDP  := $(MPEGDEMUX_OBJ)

BIN  += $(MPEGDEMUX_BIN)
MAN1 += $(MPEGDEMUX_MAN1)
CLN  += $(MPEGDEMUX_BIN) $(MPEGDEMUX_OBJ)
DIST += $(MPEGDEMUX_SRC) $(MPEGDEMUX_HDR) $(MPEGDEMUX_MAN1)

$(rel)/buffer.o:	$(rel)/buffer.c $(MPEGDEMUX_SDP)
$(rel)/getopt.o:	$(rel)/getopt.c $(MPEGDEMUX_SDP)
$(rel)/message.o:	$(rel)/message.c $(MPEGDEMUX_SDP)
$(rel)/mpegdemux.o:	$(rel)/mpegdemux.c $(MPEGDEMUX_SDP)
$(rel)/mpeg_parse.o:	$(rel)/mpeg_parse.c $(MPEGDEMUX_SDP)
$(rel)/mpeg_list.o:	$(rel)/mpeg_list.c $(MPEGDEMUX_SDP)
$(rel)/mpeg_demux.o:	$(rel)/mpeg_demux.c $(MPEGDEMUX_SDP)
$(rel)/mpeg_remux.o:	$(rel)/mpeg_remux.c $(MPEGDEMUX_SDP)
$(rel)/mpeg_scan.o:	$(rel)/mpeg_scan.c $(MPEGDEMUX_SDP)

$(rel)/mpegdemux$(EXEEXT): $(MPEGDEMUX_BDP)
	$(QP)echo "  LD     $@"
	$(QR)$(LD) -o $@ $(LDFLAGS) $(MPEGDEMUX_OBJ) $(LIBS)
