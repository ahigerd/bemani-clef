#!/usr/bin/python3

import os, sys

notetxt = '''
The .sd9 files in the root directory and the .2dx9 files in the individual track directories can be played using vgmstream (https://vgmstream.org/).
The .1 files in the individual track directories can be played using bemani2wav (https://bitbucket.org/ahigerd/bemani2wav/downloads/).
'''

for folder in sys.argv[1:]:
    if folder[-1] == '/':
        folder = folder[:-1]
    if not os.path.isdir(folder):
        continue
    album = os.path.basename(folder).split('(')[0].strip()
    print(album)
    if '(' in folder:
        releasedate = os.path.basename(folder).split('(')[1].split(')')[0]
        year = releasedate.split('-')[0]
    else:
        releasedate = None
        year = None
    tracks = {}
    for track in os.listdir(folder):
        bin_order = []
        if os.path.exists(os.path.join(folder, '!tags.m3u')):
            bin_order = [line.strip() for line in open(os.path.join(folder, '!tags.m3u'), 'r') if line.strip() and line.strip()[0] != '#']
        tpath = os.path.join(folder, track)
        if not os.path.isdir(tpath):
            if tpath.endswith('.sd9'):
                if track in bin_order:
                    tracks[' %03d' % bin_order.index(track)] = track
                else:
                    tracks['0000_%s' % track] = track
            continue
        ifs = [fn for fn in os.listdir(tpath) if fn.endswith('.1') or fn.endswith('.2dx') or fn.endswith('.2dx9') or fn.endswith('.sd9')]
        if not ifs:
            continue
        ifs.sort(key=lambda fn: (not fn.endswith('.1'), fn.endswith('.2dx9')))
        print('\t', track)
        artist, title = track.split(' - ', maxsplit=1)
        trackno = ifs[0].split('.')[0]
        tracks[trackno] = '%s/%s' % (track, ifs[0])
        with open(os.path.join(tpath, '!tags.m3u'), 'w') as m3u:
            m3u.write('# @ALBUM@ %s\n' % album)
            m3u.write('# @ALBUMARTIST@ %s\n' % 'Konami')
            if releasedate:
                m3u.write('# @DATE@ %s\n' % releasedate)
            if year:
                m3u.write('# @YEAR@ %s\n' % year)
            m3u.write('# @ARTIST@ %s\n' % artist)
            m3u.write('# @TITLE@ %s\n' % title)
            m3u.write('# @TRACK@ %s\n' % trackno)
            for fn in ifs:
                if fn.endswith('.2dx9'):
                    m3u.write('# %%TITLE%% %s (preview)\n' % title)
                m3u.write('%s\n' % fn)
    with open(os.path.join(folder, 'note.txt'), 'w') as notefile:
        notefile.write('Game: %s\nPublisher: Konami\n%s' % (os.path.basename(folder).replace('(Konami)', ''), notetxt))
    with open(os.path.join(folder, '%s.m3u8' % album), 'w') as playlist:
        for order, track in sorted(tracks.items()):
            playlist.write('%s\n' % track)
