bin_PROGRAMS = \
	diseqc \
	getpids \
	sendburst \
	set22k \
	setpid \
	setvoltage \
	stream_to_ts \
	test_av \
	test_av_play \
	test_clip \
	test_clip_teletext \
	test_clip_pes \
	test_clip_vlc \
	test_clip_vls \
	test_dvr \
	test_dvr_play \
	test_pes \
	test_sec_ne \
	test_sections \
	test_stc \
	test_stillimage \
	test_tt \
	test_vevent \
	video

test_clip_vlc_LDADD = 	-lpthread

diseqc_SOURCES = diseqc.c
getpids_SOURCES = getpids.c
sendburst_SOURCES = sendburst.c
set22k_SOURCES = set22k.c
setpid_SOURCES = setpid.c
setvoltage_SOURCES = setvoltage.c
test_av_SOURCES = test_av.c
test_av_play_SOURCES = test_av_play.c
test_clip_SOURCES = test_clip.c
test_clip_teletext = test_clip_teletext.c
test_clip_pes_SOURCES = test_clip_pes.c
test_clip_vlc_SOURCES = test_clip_vlc.c ringbuffer.c
test_clip_vls_SOURCES = test_clip_vls.c
test_dvr_SOURCES = test_dvr.c
test_dvr_play_SOURCES = test_dvr_play.c
test_pes_SOURCES = test_pes.c hex_dump.c
test_sec_ne_SOURCES = test_sec_ne.c hex_dump.c
test_sections_SOURCES = test_sections.c hex_dump.c
test_stc_SOURCES = test_stc.c
test_stillimage_SOURCES = test_stillimage.c
test_tt_SOURCES = test_tt.c hex_dump.c
test_vevent_SOURCES = test_vevent.c
video_SOURCES = video.c

AM_CFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

