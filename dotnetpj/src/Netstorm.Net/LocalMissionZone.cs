// 로컬 미션 Zone 설치의 사전 구현을 공유한다.
// 근거: cpppj `src/zacket/LocalMissionTransport.h`·`LocalMissionTransport.cpp`의 `InstallLocalMissionZone`.
// 세션 연결(F3)에서 실제 분배기·CoreData 별칭과 잇기 전까지 단독으로만 둔다.
// 2026-11-10 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.
using Netstorm.Core.Original;

namespace Netstorm.Net;

// 로컬 미션 Zone 설치 범주를 담는 네임스페이스다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.

// Zone 설치가 읽고 쓰는 전역 묶음이다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.
public sealed class LocalMissionZoneGlobals
{
    /// <summary>서버 모드 표시다. 설치 때 항상 참이 된다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Server { get; set; }

    /// <summary>방장 표시다. 설치 때 항상 참이 된다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Boss { get; set; }

    /// <summary>자신의 구독자 번호다. 단일 미션이면 1이며 바꾸지 않는다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint MySubscriber { get; set; } = 1u;

    /// <summary>대기 중인 설치 스탬프다. 설치 때 활성 스탬프로 옮긴다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint PendingStamp { get; set; }

    /// <summary>활성 설치 스탬프다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint ActiveStamp { get; set; }

    /// <summary>Zone+10의 활성 Zone 낱말이다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint ActiveZoneWord { get; set; }

    /// <summary>Zone+4의 Zone 플래그 DWORD다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint ZoneFlags { get; set; }

    /// <summary>이른 포획 종료 시각이다. 이미 돌고 있으면 설치를 거부한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public double EarlyCaptureEnd { get; set; }

    /// <summary>Zone 플래그 하위 6비트 덮어쓰기 여부다. 미션이 아닐 때만 적용한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public RawCheckingFlag AdjustZoneFlags { get; } = new();

    /// <summary>미션 월드 여부다. 참이면 Zone 플래그 덮어쓰기를 건너뛴다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool MissionWorld { get; set; }

    /// <summary>Zone 플래그 하위 6비트 덮어쓸 값이다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public byte ZoneFlagOverride { get; set; }

    /// <summary>메타맵 여부다. 참이면 현재 CoreData를 캐시 사본으로 둔다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Metamap { get; set; }

    /// <summary>현재 CoreData 10바이트다. 세션 연결 전까지 사본으로 두며 별칭은 잇지 않는다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public byte[] CurrentCoreData { get; set; } = new byte[10];

    /// <summary>메타맵에서 읽을 캐시 CoreData 10바이트다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public byte[] CachedCoreData { get; set; } = new byte[10];
}

// Zone 설치가 부르는 외부 효과 묶음이다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.
public sealed class LocalMissionZoneHooks
{
    /// <summary>로그인 제한 시간을 읽는다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<int>? LoginTimeout { get; set; }

    /// <summary>현재 게임 시각(초)을 읽는다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<double>? Seconds { get; set; }

    /// <summary>채팅 이름 표에 구독자·이름을 둔다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint, string>? StoreName { get; set; }

    /// <summary>포트에서 실제 서브를 시작한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Serve { get; set; }

    /// <summary>연산 진단 문자열을 받는다. 없으면 설치가 멈춘다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<string>? Assertion { get; set; }
}

// 단독 미션의 Zone 바이트로 Player9·전역·이름표를 채우는 설치다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.
public static class LocalMissionZone
{
    /// <summary>Zone 첫 슬롯의 위치다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public const int ZoneHead = 15;

    /// <summary>Zone 한 슬롯의 바이트 수다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public const int ZoneClient = 71;

    /// <summary>Player 수다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public const int PlayerCount = 9;

    /// <summary>Player 한 기록의 바이트 수다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    public const int PlayerBytes = 180;

    /// <summary>PATCH 004b51e0의 Zone 512·네 슬롯 작성과 0040a1e0의 86바이트 길이 확정을 둔다. 세션 연결 전까지 단독 설치만 쓴다.</summary>
    /// <param name="zone">읽을 Zone 바이트다. 15바이트 이상이어야 한다.</param>
    /// <param name="players">쓸 9개 Player 바이트다. 1620바이트여야 한다.</param>
    /// <param name="player">갱신할 현재 Player 상태다.</param>
    /// <param name="globals">갱신할 전역 묶음이다.</param>
    /// <param name="hooks">부를 외부 효과 묶음이다. 모든 경계는 실제 소비자여야 한다.</param>
    /// <param name="port">서브를 시작할 포트다.</param>
    /// <exception cref="ArgumentException">Zone·Player 크기 부족, 클라이언트 수·소유자 범위 오류다.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Zone 슬롯이 버퍼를 넘는다.</exception>
    /// <exception cref="InvalidOperationException">필수 효과가 묶이지 않았다.</exception>
    public static void Install(
        ReadOnlySpan<byte> zone,
        Span<byte> players,
        PlayerWorldState player,
        LocalMissionZoneGlobals globals,
        LocalMissionZoneHooks hooks,
        uint port)
    {
        ArgumentNullException.ThrowIfNull(player);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (players.Length != PlayerCount * PlayerBytes || zone.Length < ZoneHead)
        {
            throw new ArgumentException("로컬 Zone 설치의 Player9·Zone 크기 부족");
        }

        int clients = zone[14];
        if (clients <= 0 || clients > 8)
        {
            Need(hooks.Assertion)("numClients > 0 && numClients < 9");
        }

        if (ZoneHead + (long)clients * ZoneClient > zone.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(zone), "로컬 Zone 설치의 client 기록 범위 부족");
        }

        globals.Server = true;
        globals.Boss = true;
        globals.ActiveStamp = globals.PendingStamp;
        globals.ActiveZoneWord = Read(zone, 10);
        globals.ZoneFlags = Read(zone, 4);
        if (globals.AdjustZoneFlags && !globals.MissionWorld)
        {
            globals.ZoneFlags = (globals.ZoneFlags & ~0x3fu) | ((uint)globals.ZoneFlagOverride & 0x3fu);
        }

        if (globals.EarlyCaptureEnd != 0.0)
        {
            Need(hooks.Assertion)("!earlyCaptureTimer.isRunning()");
        }

        int timeout = Need(hooks.LoginTimeout)();
        int delay = unchecked((int)(unchecked((uint)timeout) + 120u));
        globals.EarlyCaptureEnd = delay + Need(hooks.Seconds)();

        // Zone의 클라이언트 수만큼 Player 기록·이름표를 방문한다.
        for (uint index = 0; index < (uint)clients; ++index)
        {
            int offset = ZoneHead + (int)index * ZoneClient;
            if (offset + ZoneClient > zone.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(zone), "로컬 Zone 설치의 client 슬롯 범위 부족");
            }

            if (Read(zone, offset + 1) == 0u)
            {
                Need(hooks.Assertion)("zac->subId");
            }

            uint owner = (uint)zone[offset] + 1u;
            if (owner < 1u || owner >= (uint)PlayerCount)
            {
                throw new ArgumentOutOfRangeException(nameof(zone), "로컬 Zone 설치의 Player 번호 초과");
            }

            int record = (int)owner * PlayerBytes;
            if (Read(zone, offset + 1) == globals.MySubscriber)
            {
                player.TemplateOwner = owner;
                player.BattleOwner = owner;
                player.PlayerKnown = 1u;
                if (globals.Metamap)
                {
                    if (globals.CachedCoreData.Length != 10)
                    {
                        throw new ArgumentException("로컬 Zone 설치의 metamap coreData 10바이트 부족");
                    }

                    globals.CurrentCoreData = (byte[])globals.CachedCoreData.Clone();
                }
                else
                {
                    // 세션 연결 전까지 별칭 대신 사본을 둔다. 실제 분배기 연결 때 같은 span 별칭으로 바꾼다.
                    globals.CurrentCoreData = players.Slice(record + 0x6c, 10).ToArray();
                }

                Put(players, record + 0x50, Read(players, record + 0x50) | 8u);
            }

            Copy(players, record + 12, Text(zone, offset + 9));
            Need(hooks.StoreName)(Read(zone, offset + 1), Text(players, record + 12));
            Put(players, record + 0x2c, Read(zone, offset + 1));
            if (Read(players, record + 0x2c) == 0u)
            {
                Need(hooks.Assertion)("p->subscriberId");
            }

            Copy(players, record + 0x30, Text(zone, offset + 41));
            if ((zone[offset + 5] & 1u) != 0u)
            {
                Put(players, record + 0x50, Read(players, record + 0x50) | 0x200u);
            }
        }

        Need(hooks.Serve)(port);
    }

    /// <summary>원본 비정렬 little endian 값을 읽는다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset < 0 || offset + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 Zone 원문 읽기 범위 오류");
        }

        uint value = 0u;
        // 낮은 바이트부터 같은 폭의 원본 비트를 모은다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>주변 stale 바이트를 유지하며 원본 DWORD만 쓴다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        if (offset < 0 || offset + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 Zone 원문 쓰기 범위 오류");
        }

        // 원본 little endian의 바이트 위치만 변경한다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[offset + index] = unchecked((byte)(value >> (8 * index)));
        }
    }

    /// <summary>원본 C문자열의 첫 NUL까지만 읽는다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static string Text(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset < 0 || offset > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 Zone 문자열 위치 오류");
        }

        int end = offset;
        // 끝까지 NUL이 없으면 호스트 밖 읽기로 보고 거부한다.
        while (end < bytes.Length && bytes[end] != 0)
        {
            ++end;
        }

        if (end >= bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 Zone 원문 문자열 NUL 누락");
        }

        return System.Text.Encoding.Latin1.GetString(bytes[offset..end]);
    }

    /// <summary>원본 strcpy처럼 NUL까지 복사한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="text">쓸 문자열이다.</param>
    private static void Copy(Span<byte> bytes, int offset, string text)
    {
        int nul = text.IndexOf('\0');
        if (nul >= 0)
        {
            text = text.Substring(0, nul);
        }

        byte[] encoded = System.Text.Encoding.Latin1.GetBytes(text);
        if (offset < 0 || offset >= bytes.Length || encoded.Length >= bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 Zone strcpy 물리 저장소 부족");
        }

        // 선언 필드 폭을 넘어도 실제 물리 저장소 안에서는 원본 쓰기를 유지한다.
        encoded.CopyTo(bytes[offset..]);
        bytes[offset + encoded.Length] = 0;
    }

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-10 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("로컬 Zone 설치의 선택된 실제 효과 미연결");
        }

        return callback;
    }
}
