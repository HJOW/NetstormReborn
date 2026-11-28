using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은96BYTE 채널에 실제 파일 헤더와 버퍼 생성을 원문 순서로 연결한다.
namespace Netstorm.Client.Audio;

/// <summary>cpppj SoundMusic.cpp의 실제 Channel.Open·EnsureBuffer를 기존 읽기/시작/정지 소유자에 추가한다.</summary>
public sealed partial class MusicChannel
{
    /// <summary>원문 CreateSoundBuffer의 실제 기능 비트다.</summary>
    public const uint BufferFlags = 0xe2;
    /// <summary>표본 길이와 무관하게 원문이 확보하는 실제 음악 링 버퍼 크기다.</summary>
    public const uint BufferBytes = 0x2af80;

    /// <summary>같은 실제 파일 조회/열기 뒤 실패 반환의 부분 헤더도 현재 채널에 반영하고 판본별 순서로 길이를 계산한다.</summary>
    public bool Open(string name, MusicDirectories directories, MusicOpenHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Find is null || hooks.Open is null) throw new ArgumentException("음악 파일 열기의 필수 실제 장치 누락", nameof(hooks));
        using var guard = new Guard(this);
        string? found = MusicFileLookup.FindMusicFile(name, directories, hooks.Find);
        if (found is null)
        {
            _hooks.Log?.Invoke(0x13, "Missing music: \"(null)\"\n");
            return false;
        }
        var header = new MusicFileHeader
        {
            File = _state.Field(MusicField.File),
            Length = _state.Field(MusicField.Length),
            DataOffset = _state.Field(MusicField.DataOffset),
        };
        _state.Raw().Span.Slice(0x28, 18).CopyTo(header.Format);
        bool opened = hooks.Open(found, header);
        _state.SetField(MusicField.File, header.File);
        _state.SetField(MusicField.Length, header.Length);
        _state.SetField(MusicField.DataOffset, header.DataOffset);
        header.Format.CopyTo(_state.Raw().Span.Slice(0x28, 18));
        if (!opened) { Log("Failed to open music file: \"" + found + "\"\n"); return false; }
        uint rate = BinaryPrimitives.ReadUInt32LittleEndian(header.Format.AsSpan(4, 4));
        uint channels = BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(2, 2));
        uint bits = BinaryPrimitives.ReadUInt16LittleEndian(header.Format.AsSpan(14, 2));
        if (header.Length > int.MaxValue || rate == 0 || channels == 0 || bits == 0)
            throw new ArgumentOutOfRangeException(nameof(hooks), "음악 길이 계산 인자가 올바르지 않다");
        double duration;
        if (_edition == OriginalEdition.Patch1078)
        {
            duration = (double)header.Length / (bits * 0.125);
            duration /= rate;
            duration /= channels;
        }
        else
        {
            double denominator = (bits * 0.125) * channels;
            denominator *= rate;
            duration = header.Length / denominator;
        }
        BinaryPrimitives.WriteUInt64LittleEndian(_state.Raw().Span.Slice(0x18, 8), BitConverter.DoubleToUInt64Bits(duration));
        return true;
    }

    /// <summary>기존 버퍼는 보존하고 없는 경우만 원문 고정 크기로 생성한다. 실패 HRESULT도 ref 출력 토큰을 보존한다.</summary>
    public bool EnsureBuffer(MusicBufferHooks? hooks)
    {
        using (var guard = new Guard(this))
        {
            if (_state.Field(MusicField.Buffer) != 0) return true;
            var create = hooks?.Create ?? throw new ArgumentException("음악 버퍼 생성의 실제 장치 누락", nameof(hooks));
            uint buffer = _state.Field(MusicField.Buffer);
            int result = create(_state.Raw().Span.Slice(0x28, 18), BufferFlags, BufferBytes, ref buffer);
            _state.SetField(MusicField.Buffer, buffer);
            if (result >= 0) { _state.SetField(MusicField.BufferBytes, BufferBytes); return true; }
        }
        Log("Failed to create a music buffer.\n");
        return false;
    }

    /// <summary>같은 실제 채널의 파일/버퍼 장치를 공개 음악 선택에 연결하고 함수 슬롯 사본으로 호출 대상을 보존한다.</summary>
    public MusicPlaybackDevices BindPlaybackDevices(MusicOpenHooks files, MusicBufferHooks buffers)
    {
        ArgumentNullException.ThrowIfNull(files);
        ArgumentNullException.ThrowIfNull(buffers);
        var open = files.Open;
        var fallback = files.Fallback;
        var copiedBuffers = buffers.Copy();
        return new()
        {
            Active = Active,
            Open = (name, directories, find) => Open(name, directories, new() { Find = find, Open = open, Fallback = fallback }),
            EnsureBuffer = () => EnsureBuffer(copiedBuffers),
            Start = Start,
            Stop = Stop,
        };
    }
}
