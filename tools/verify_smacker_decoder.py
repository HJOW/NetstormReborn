#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""게임/창 없이 실제 SMK의 모든 RGB 프레임과 PCM을 독립 FFmpeg 출력과 대조한다."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


def sha(path):
    """원문 또는 도구의 실제 파일 바이트 SHA256을 구한다."""
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1048576), b''): digest.update(block)  # 큰 영상 원문도 순차 읽기만 한다.
    return digest.hexdigest()


def main():
    """먼저 독립 기준 출력을 고정한 뒤 제품 해시를 읽어 비교하며 최초 출력은 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--media', type=Path, required=True); parser.add_argument('--decoded', type=Path, required=True); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    media = args.media.resolve(); decoded = args.decoded.resolve(); output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    ffmpeg = shutil.which('ffmpeg'); ffprobe = shutil.which('ffprobe')
    if not ffmpeg or not ffprobe: raise RuntimeError('독립 검증용 FFmpeg/ffprobe가 필요합니다')
    version = subprocess.check_output([ffmpeg, '-version'], text=True, encoding='utf-8').splitlines()[0]
    probe_command = [ffprobe, '-v', 'error', '-show_entries', 'stream=index,codec_name,width,height,r_frame_rate,sample_rate,channels,sample_fmt,duration', '-of', 'json', str(media)]
    probe = json.loads(subprocess.check_output(probe_command, text=True, encoding='utf-8')); (output / 'probe.json').write_text(json.dumps(probe, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    video_path = output / 'frames.sha256'; audio_path = output / 'audio0.s16le'
    commands = [[ffmpeg, '-nostdin', '-n', '-v', 'error', '-threads', '2', '-i', str(media), '-map', '0:v:0', '-an', '-pix_fmt', 'rgb24', '-fps_mode', 'passthrough', '-f', 'framehash', '-hash', 'sha256', str(video_path)],
                [ffmpeg, '-nostdin', '-n', '-v', 'error', '-threads', '2', '-i', str(media), '-map', '0:a:0', '-vn', '-c:a', 'pcm_s16le', '-f', 's16le', str(audio_path)]]  # RGB/원래16비트 PCM을 재보정·재표본화하지 않는다.
    for index, command in enumerate(commands):  # 기대 출력 계산에 제품 디코더의 결과를 사용하지 않는다.
        with (output / f'ffmpeg-{index}.log').open('wb') as stream: subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, check=True, timeout=180)
    expected = [line.split(',')[-1].strip() for line in video_path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    expected_audio = {'bytes': audio_path.stat().st_size, 'sha256': sha(audio_path)}
    (output / 'independent-reference.json').write_text(json.dumps({'mediaSha256': sha(media), 'ffmpegVersion': version, 'ffmpegSha256': sha(ffmpeg), 'commands': commands, 'frames': len(expected), 'frameListSha256': sha(video_path), 'audio0': expected_audio}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    actual_meta = json.loads((decoded / 'metadata.json').read_text(encoding='utf-8')); actual = [line.split('\t')[1] for line in (decoded / 'frames.tsv').read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    differences = [index for index, pair in enumerate(zip(expected, actual)) if pair[0] != pair[1]]
    audio_stream = next(stream for stream in probe['streams'] if stream['codec_name'] == 'smackaudio'); video_stream = next(stream for stream in probe['streams'] if stream['codec_name'] == 'smackvideo')
    assert audio_stream['sample_fmt'] == 's16' and int(audio_stream['channels']) == 2 and int(audio_stream['sample_rate']) == 22050, audio_stream
    checks = {'frameCounts': len(expected) == len(actual) == actual_meta['decodedFrames'], 'allRgbFrameHashes': not differences,
              'dimensions': actual_meta['width'] == video_stream['width'] and actual_meta['height'] == video_stream['height'],
              'audioBytes': actual_meta['audio'][0]['bytes'] == expected_audio['bytes'], 'audioSha256': actual_meta['audio'][0]['sha256'] == expected_audio['sha256']}
    report = {'passed': all(checks.values()), 'checks': checks, 'frames': len(expected), 'frameDifferences': differences, 'actualMetadata': actual_meta, 'expectedAudio': expected_audio, 'mediaSha256': sha(media), 'ffmpegVersion': version, 'gameOrWindowExecuted': False}
    (output / 'comparison.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n'); print(json.dumps({'passed': report['passed'], 'frames': len(expected), 'differences': len(differences), 'checks': checks}, ensure_ascii=False)); assert report['passed'], checks


if __name__ == '__main__': main()
