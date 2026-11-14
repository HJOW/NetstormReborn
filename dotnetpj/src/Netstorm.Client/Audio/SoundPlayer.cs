using System.Text;
using Netstorm.Core.Original;

// 현재 소리 표·장치·카메라의 원문 재생/정지/복제·음량 순서를 보존한다.
namespace Netstorm.Client.Audio;

/// <summary>CPP SoundPlayer의 실제 장치 호출과 이름/위치 기반 재생 계층이다.</summary>
public sealed class SoundPlayer
{
    /// <summary>원본 오류 기록 채널이다.</summary>
    private const uint LogError = 9;
    /// <summary>원본 소리 기록 채널이다.</summary>
    private const uint LogSound = 0x1d;
    /// <summary>장치의 재생 비트다.</summary>
    private const uint PlayingBit = 1;
    /// <summary>장치의 소실 비트다.</summary>
    private const uint LostBit = 2;
    /// <summary>장치의 반복 비트다.</summary>
    private const uint LoopBit = 4;
    /// <summary>이름/버퍼/복제 사슬이 공유하는 같은 물리 표다.</summary>
    private readonly SoundList _list;
    /// <summary>모든 관문이 소비 시점의 값을 읽는 현재 전역이다.</summary>
    private readonly SoundState _state;
    /// <summary>원문 함수 객체처럼 값 복사한 필수 실제 장치다.</summary>
    private readonly SoundDeviceHooks _hooks;
    /// <summary>선택적 정상 복귀 원문 보고다.</summary>
    private readonly Action<string, int>? _report;
    /// <summary>실행 중 미연결을 만나지 않도록 원문 필수 여덟 장치를 구성 때 확인한다.</summary>
    public SoundPlayer(SoundList list, SoundState state, SoundDeviceHooks hooks, Action<string, int>? report = null)
    {
        ArgumentNullException.ThrowIfNull(list); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Status is null || hooks.Play is null || hooks.SetPosition is null || hooks.SetVolume is null || hooks.SetPan is null || hooks.Stop is null || hooks.Duplicate is null || hooks.Load is null)
            throw new ArgumentException("소리 재생 계층의 필수 장치 효과 누락", nameof(hooks));
        _list = list; _state = state; _hooks = hooks.Copy(); _report = report;
    }
    /// <summary>원문의 투영/판정/좌우/음량 위치마다 현재 카메라/영역을 값으로 읽는다.</summary>
    public SoundView CurrentView() => _state.CurrentView?.Invoke() ?? _state.View;
    /// <summary>원문 표의 실제 범위 검사를 통해 현재 머리 필드를 읽는다.</summary>
    private uint Get(uint entry, SoundField field) => _list.Field(entry, field);
    /// <summary>원문 표의 현재 머리 필드만 쓴다.</summary>
    private void Set(uint entry, SoundField field, uint value) => _list.SetField(entry, field, value);
    /// <summary>선택적 원문 채널/완성된 문장을 기록한다.</summary>
    private void Log(uint channel, string text) => _hooks.Log?.Invoke(channel, text);
    /// <summary>원문 보고가 복귀하면 현재 흐름을 이어 간다.</summary>
    private void Report(string expression, int line) => _report?.Invoke(expression, line);
    /// <summary>현재 음량+전체 음량-항목 감쇠를DWORD로 감고[-10000,0]으로 자른다.</summary>
    private int Gain(uint entry, int volume) => Math.Clamp(unchecked((int)((uint)volume + (uint)_state.MasterVolume - Get(entry, SoundField.Attenuation))), SoundConstants.Minimum, 0);
    /// <summary>실제 버퍼만 장치 상태를 조회하고 미적재 진단/소리없음 관문을 보존한다.</summary>
    private uint BufferPlaying(uint entry)
    {
        uint buffer = Get(entry, SoundField.Buffer);
        if (buffer == 0) { Report("buffer", 0x82); throw new InvalidOperationException("버퍼가 없는 소리 항목의 상태 조회"); }
        if (buffer == SoundConstants.SilentBuffer) return 0;
        return _hooks.Status!(buffer) & PlayingBit;
    }
    /// <summary>현재 버퍼 소실 진단→정지→위치0→현재 재생 수 감소 순서다.</summary>
    private void StopBuffer(uint entry)
    {
        uint buffer = Get(entry, SoundField.Buffer);
        if (buffer == 0) { Report("buffer", 0x9b); Report("!isLost()", 0x9c); throw new InvalidOperationException("버퍼가 없는 소리 항목의 정지"); }
        if (buffer == SoundConstants.SilentBuffer) throw new InvalidOperationException("소리 없음 항목의 정지");
        if ((_hooks.Status!(buffer) & LostBit) != 0) Report("!isLost()", 0x9c);
        _hooks.Stop!(buffer); _hooks.SetPosition!(buffer, 0); _state.Playing = unchecked(_state.Playing - 1);
    }
    /// <summary>실제 재생 중일 때만 정지하며 원문 두 번의 상태 조회를 합치지 않는다.</summary>
    public void Stop(uint sound)
    {
        if (sound == 0 || Get(sound, SoundField.Buffer) == 0 || BufferPlaying(sound) == 0) return;
        StopBuffer(sound);
    }
    /// <summary>준비→항목→옵션→버퍼 순서 관문 뒤 실제 상태를 반환한다.</summary>
    public uint IsPlaying(uint sound)
    {
        if (!_state.Initialized || sound == 0 || !_state.Enabled || Get(sound, SoundField.Buffer) == 0) return 0;
        return BufferPlaying(sound);
    }
    /// <summary>복제 사슬 대신 전체 물리 항목을 순회해 현재 장치 상태로 개수를 다시 센다.</summary>
    public int Recount()
    {
        _state.Playing = 0;
        // 이름 길이는 각 순회의 처음에 저장하고 보고/장치 효과 뒤에도 그 길이로 다음 물리 항목에 간다.
        for (uint sound = SoundConstants.ListBase; ;)
        {
            ReadOnlySpan<byte> name = _list.NameBytes(sound); if (name.IsEmpty) break;
            if (_list.Edition != OriginalEdition.Patch1078 && name[^1] != (byte)'v') Report("s->isRobust()", 0x1e7);
            uint buffer = Get(sound, SoundField.Buffer);
            if (buffer != 0 && buffer != SoundConstants.SilentBuffer && (_hooks.Status!(buffer) & PlayingBit) != 0) _state.Playing = unchecked(_state.Playing + 1);
            sound = unchecked(sound + SoundConstants.HeaderBytes + (uint)name.Length + 1);
        }
        return _state.Playing;
    }
    /// <summary>이름을 먼저 등록/조회하고 유일성 보고 뒤 원본 항목만 정지한다.</summary>
    public void StopByName(string name)
    {
        uint sound = _list.Lookup(name);
        if (Get(sound, SoundField.Root) != sound || Get(sound, SoundField.Next) != 0) Report("sound->isUnique()", _list.Edition == OriginalEdition.Patch1078 ? 0x3f8 : 0x3f3);
        Stop(sound);
    }
    /// <summary>옵션/준비 검사보다 이름 조회를 먼저 수행한다.</summary>
    public uint IsPlayingByName(string name) => IsPlaying(_list.Lookup(name));
    /// <summary>준비/이름/옵션 관문 뒤0은정지, 비영은필요 시 재생하고 성공 여부와 무관하게 on을 반환한다.</summary>
    public int SetNamePlaying(int on, string? name, uint loop, int volume)
    {
        if (!_state.Initialized || name is null || !_state.Enabled) return 0;
        if (on == 0) { StopByName(name); return 0; }
        uint sound = _list.Lookup(name);
        if (Get(sound, SoundField.Buffer) == 0 || BufferPlaying(sound) == 0) PlayByName(name, loop, volume, 0, 0, 0);
        return on;
    }
    /// <summary>물리 표의 복제까지 실제 재생/반복 상태를 따로 조회하고 반복 버퍼만 정지한다.</summary>
    public void StopLoops()
    {
        // 각 항목의 현재 이름 길이를 보존해 다음 위치로 진행한다.
        for (uint sound = SoundConstants.ListBase; ;)
        {
            ReadOnlySpan<byte> name = _list.NameBytes(sound); if (name.IsEmpty) break;
            uint buffer = Get(sound, SoundField.Buffer);
            if (buffer != 0 && buffer != SoundConstants.SilentBuffer && (_hooks.Status!(buffer) & PlayingBit) != 0 && (_hooks.Status!(buffer) & LoopBit) != 0) StopBuffer(sound);
            sound = unchecked(sound + SoundConstants.HeaderBytes + (uint)name.Length + 1);
        }
    }
    /// <summary>보류 깊이가비영이면예약만쓰고, 아니면전체음량을저장한뒤준비된모든실제버퍼를갱신한다.</summary>
    public void SetMasterVolume(int volume)
    {
        if (_state.VolumeHoldDepth != 0) { _state.PendingMasterVolume = volume; return; }
        _state.MasterVolume = volume; if (!_state.Initialized) return;
        // 현재 재생 여부와 무관하게 모든 적재 버퍼를 원문 물리 순서로 갱신한다.
        for (uint sound = SoundConstants.ListBase; ;)
        {
            ReadOnlySpan<byte> name = _list.NameBytes(sound); if (name.IsEmpty) break;
            if (_list.Edition != OriginalEdition.Patch1078 && name[^1] != (byte)'v') Report("s->isRobust()", 0x442);
            uint buffer = Get(sound, SoundField.Buffer);
            if (buffer != 0 && buffer != SoundConstants.SilentBuffer)
            {
                int gain = unchecked((int)(Get(sound, SoundField.Volume) - Get(sound, SoundField.Attenuation) + (uint)_state.MasterVolume));
                _ = _hooks.SetVolume!(buffer, gain);
            }
            sound = unchecked(sound + SoundConstants.HeaderBytes + (uint)name.Length + 1);
        }
    }
    /// <summary>현재 한도/원본 적재/복제 사슬을 거쳐 실제 선택 버퍼에 우선·일련·음량을 쓰고 재생한다.</summary>
    public uint Play(uint sound, uint loop, int volume, int pan, uint priority, int limit)
    {
        if (!_state.Initialized || sound == 0 || !_state.Enabled) return 0;
        if (priority == 0 && _state.MaxPlaying != 0 && _state.Playing >= _state.MaxPlaying) return 0;
        string file = _list.Edition == OriginalEdition.Patch1078 ? @".\Sound.cpp" : "sound.cpp";
        // 복제 실패 위치의 원문 문장과 줄을 그대로 기록한다.
        void Failed(int line) => Log(LogError, $"Failed to duplicate sound buffer [{file} line {line}]\n");
        if (Get(sound, SoundField.Buffer) == 0)
        {
            uint root = Get(sound, SoundField.Root);
            if (Get(root, SoundField.Buffer) == 0)
            {
                SoundLoad loaded = _hooks.Load!(_list.Name(root)); Set(root, SoundField.Buffer, loaded.Buffer);
                if (loaded.Buffer != 0) Set(root, SoundField.Attenuation, unchecked((uint)loaded.Attenuation));
            }
            if (Get(sound, SoundField.Buffer) == 0)
            {
                uint restored = 0;
                if (_hooks.Duplicate!(Get(Get(sound, SoundField.Root), SoundField.Buffer), ref restored) < 0) { Failed(0x310); Set(sound, SoundField.Buffer, 0); return sound; }
                Set(sound, SoundField.Buffer, restored); Log(LogSound, $"Restored sound {_list.Name(sound)}\n");
            }
        }
        if (Get(sound, SoundField.Buffer) == SoundConstants.SilentBuffer) return sound;
        uint entry = sound, buffer;
        if (BufferPlaying(sound) == 0) buffer = Get(sound, SoundField.Buffer);
        else
        {
            uint walk = Get(sound, SoundField.Root), last = 0; int count = 0;
            // 현재 복제 사슬에서 실제로 재생 중인 항목만 원문 순서로 센다.
            while (walk != 0 && Get(walk, SoundField.Buffer) != 0 && BufferPlaying(walk) != 0) { last = walk; walk = Get(walk, SoundField.Next); count = unchecked(count + 1); }
            if (limit != 0 && count >= limit) return 0;
            if (walk != 0)
            {
                buffer = Get(walk, SoundField.Buffer);
                if (buffer == 0)
                {
                    uint refill = 0;
                    if (_hooks.Duplicate!(Get(sound, SoundField.Buffer), ref refill) < 0) { Failed(0x33c); Set(walk, SoundField.Buffer, 0); return walk; }
                    Set(walk, SoundField.Buffer, refill); buffer = refill;
                }
                entry = walk;
            }
            else
            {
                uint copy = 0;
                if (_hooks.Duplicate!(Get(sound, SoundField.Buffer), ref copy) < 0) { Failed(0x348); return sound; }
                if (last == 0) throw new InvalidOperationException("소리 항목의 원본 연결이 비어 있습니다");
                entry = _list.AppendDuplicate(sound, last, copy); buffer = copy;
            }
        }
        Set(entry, SoundField.Priority, priority); uint serial = _state.Serial; _state.Serial = unchecked(serial + 1); Set(entry, SoundField.Serial, serial); Set(entry, SoundField.Volume, unchecked((uint)volume));
        if (_hooks.SetVolume!(buffer, Gain(entry, volume)) < 0) Log(LogSound, "failed to set buffer volume\n");
        _ = _hooks.SetPan!(buffer, pan); _hooks.Play!(buffer, loop != 0 ? 1u : 0u); _state.Playing = unchecked(_state.Playing + 1); return entry;
    }
    /// <summary>현재 이름/장치/화면 관문과 반복 버퍼의 현재 위치 음량/좌우를 원문 순서로 갱신한다.</summary>
    public uint PlayLoopAt(float x, float y, uint current, uint sound)
    {
        if (!_state.Enabled || !_state.Device) return 0;
        if (current != 0 && sound != 0)
        {
            if (SoundList.SameName(_list.NameBytes(current), _list.NameBytes(sound))) sound = 0;
            else { StopBuffer(current); current = 0; }
        }
        SoundPoint point = SoundGeometry.ToScreen(CurrentView(), x, y); bool visible = SoundGeometry.OnScreen(CurrentView(), point);
        if (visible && !(current != 0 && Get(current, SoundField.Buffer) != 0 && BufferPlaying(current) != 0)) current = Play(sound != 0 ? sound : current, 1, 0, 0, 0, 1);
        if (current == 0) return 0;
        if (!visible)
        {
            if (Get(current, SoundField.Buffer) == 0) return current;
            if (BufferPlaying(current) != 0) { StopBuffer(current); return 0; }
        }
        if (Get(current, SoundField.Buffer) == 0 || BufferPlaying(current) == 0) return current;
        bool patch = _list.Edition == OriginalEdition.Patch1078; string file = patch ? @".\Sound.cpp" : "sound.cpp";
        SoundView panView = CurrentView(); int pan = SoundGeometry.Pan(panView, point, _state.SwapSpeakers); int volume = SoundGeometry.Volume(CurrentView(), point);
        Set(current, SoundField.Volume, unchecked((uint)volume)); uint buffer = Get(current, SoundField.Buffer);
        if (_hooks.SetVolume!(buffer, Gain(current, volume)) != 0) Log(LogError, $"Failed to set sound buffer volume [file {file}, line {(patch ? 0x3d0 : 0x3cb)}]\n");
        if (_hooks.SetPan!(buffer, pan) != 0) Log(LogError, $"Failed to set sound buffer pan [file {file}, line {(patch ? 0x3d5 : 0x3d0)}]\n");
        return current;
    }
    /// <summary>현재 지도로 투영/화면 판정 뒤 다시 현재 좌우→음량을 읽어 실제 재생에 전달한다.</summary>
    public uint PlayOnceAt(float x, float y, uint sound, uint loop, uint priority)
    {
        SoundPoint point = SoundGeometry.ToScreen(CurrentView(), x, y); if (!SoundGeometry.OnScreen(CurrentView(), point)) return 0;
        SoundView panView = CurrentView(); int pan = SoundGeometry.Pan(panView, point, _state.SwapSpeakers); int volume = SoundGeometry.Volume(CurrentView(), point);
        return Play(sound, loop, volume, pan, priority, 0);
    }
    /// <summary>준비/옵션과 무관하게 이름을 조회하고 미적재일 때만 실제 파일/버퍼를 적재한다.</summary>
    public uint Preload(string name)
    {
        uint entry = _list.Lookup(name);
        if (Get(entry, SoundField.Buffer) == 0)
        {
            SoundLoad loaded = _hooks.Load!(name); Set(entry, SoundField.Buffer, loaded.Buffer); if (loaded.Buffer != 0) Set(entry, SoundField.Attenuation, unchecked((uint)loaded.Attenuation));
        }
        return entry;
    }
    /// <summary>반복이면 먼저 유일성 보고를 하고 이름을 다시 조회해 현재 재생에 넘긴다.</summary>
    public uint PlayByName(string name, uint loop, int volume, int pan, uint priority, int limit)
    {
        if (loop != 0)
        {
            uint entry = _list.Lookup(name); if (Get(entry, SoundField.Root) != entry || Get(entry, SoundField.Next) != 0) Report("qq", _list.Edition == OriginalEdition.Patch1078 ? 0x3ef : 0x3ea);
        }
        return Play(_list.Lookup(name), loop, volume, pan, priority, limit);
    }
    /// <summary>이름을 먼저 조회하고 위치 재생의 우선0으로 전달한다.</summary>
    public uint PlayNameAt(float x, float y, string name, uint loop) => PlayOnceAt(x, y, _list.Lookup(name), loop, 0);
}
