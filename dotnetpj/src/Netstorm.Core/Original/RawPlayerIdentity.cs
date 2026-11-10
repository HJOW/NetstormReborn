// Player/Subscriber/이름/주소 조회의 실제 60바이트 레코드와 두 판본의 검색 순서를 복원한다.
// 근거: cpppj `src/o/RawPlayerIdentity.h`·`RawPlayerIdentity.cpp`(PATCH004841c0/CD00401610 계열).
// 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Core.Original;

// Player 식별자 모듈을 담는 네임스페이스다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.

// PlayerFinder의 물리 배치를 담는 상수 클래스다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
public static class PlayerIdentityValues
{
    /// <summary>조회 레코드 전체 60바이트다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int RecordBytes = 60;

    /// <summary>이름 시작 오프셋 8이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int NameOffset = 8;

    /// <summary>이름 본문 40바이트다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int NameBytes = 40;

    /// <summary>이름 조회 폭 41이다. 주소 첫 바이트의 NUL까지 읽는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int TerminatedNameBytes = 41;

    /// <summary>주소 DWORD 오프셋 48이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int AddressOffset = 48;

    /// <summary>모드 DWORD 오프셋 52이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int ModeOffset = 52;

    /// <summary>채팅 한 행 48바이트다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int ChatStride = 48;

    /// <summary>채팅 행 수 100이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int ChatCount = 100;

    /// <summary>metamap 기록 수 40이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int MetaCount = 40;

    /// <summary>Player 기록 수 9다. 0번은 검색하지 않는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public const int PlayerCount = 9;
}

// 현재 전역을 읽는 실제 연결이다. 월드/설정과 별도의 로컬 정보 사본을 만들지 않는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
public sealed class PlayerIdentityReads
{
    /// <summary>PATCH 540c70·CD 50f6c8의 현재 로컬 소유자다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>PATCH 540c74·CD 50f6cc의 현재 구독자다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<uint>? Subscriber { get; set; }

    /// <summary>PATCH 5953fc·CD 5490d8의 현재 주소다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<uint>? Address { get; set; }

    /// <summary>PATCH 540ca8·CD 50f6d0이 가리키는 현재 원본 바이트 문자열이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<string>? Name { get; set; }

    /// <summary>전투 모드 조회다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<bool>? Battle { get; set; }

    /// <summary>편집 모드 조회다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<bool>? Editor { get; set; }

    /// <summary>metamap 모드 조회다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<bool>? Metamap { get; set; }

    /// <summary>이름 조회 실패 때 쓸 현재 SOMEONE 문자열이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<string>? UnknownName { get; set; }

    /// <summary>실제 메타 분기에서만 현재 기록을 빌린다. 미공급이면 고정 저장소를 유지한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public Func<byte[]>? MetaRecords { get; set; }
}

// PATCH004841c0/CD00401610과 실제 초기화/네 getter다. 채팅·Player·metamap 기록을 직접 빌린다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
public sealed class RawPlayerIdentity
{
    /// <summary>현재 원본 판본이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>실제 9개 Player 원문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    private readonly byte[] _players;

    /// <summary>실제 40개 metamap 기록이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    private readonly byte[] _meta;

    /// <summary>기존 실제 100×48 채팅 기록이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    private readonly byte[] _chat;

    /// <summary>같은 현재 전역/설정 조회다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    private readonly PlayerIdentityReads _reads;

    /// <summary>저장소 크기는 생성 시 확인하고 실제 레코드 검색/효과는 호출 때 실행한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="edition">현재 원본 판본이다.</param>
    /// <param name="players">실제 9개 Player 원문이다.</param>
    /// <param name="meta">실제 40개 metamap 기록이다. 비어 있으면 해당 분기 진입 시 거부한다.</param>
    /// <param name="chat">기존 실제 100×48 채팅 기록이다.</param>
    /// <param name="reads">같은 현재 전역의 실제 조회다.</param>
    /// <exception cref="ArgumentException">공유 저장소 크기 오류다.</exception>
    public RawPlayerIdentity(OriginalEdition edition, byte[] players, byte[] meta, byte[] chat, PlayerIdentityReads reads)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(meta);
        ArgumentNullException.ThrowIfNull(chat);
        ArgumentNullException.ThrowIfNull(reads);
        int playerStride = edition == OriginalEdition.Patch1078 ? 180 : 172;
        int metaStride = edition == OriginalEdition.Patch1078 ? 460 : 296;
        if (players.Length != PlayerIdentityValues.PlayerCount * playerStride
            || (meta.Length != 0 && meta.Length != PlayerIdentityValues.MetaCount * metaStride)
            || chat.Length != PlayerIdentityValues.ChatCount * PlayerIdentityValues.ChatStride)
        {
            throw new ArgumentException("Player 식별자 공유 저장소 크기 오류", nameof(players));
        }

        _edition = edition;
        _players = players;
        _meta = meta;
        _chat = chat;
        _reads = reads;
    }

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("Player 식별자의 선택된 실제 장치 미연결");
        }

        return callback;
    }

    /// <summary>원본 little endian DWORD를 부호 변경 없이 읽는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="offset">읽을 바이트 위치다.</param>
    private static uint Read(byte[] bytes, int offset)
    {
        if (offset < 0 || offset + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "Player 식별자 필드 범위 오류");
        }

        uint value = 0u;
        // 낮은 바이트부터 원본 폭만 조립하는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>현재 필드 밖의 표식 바이트를 보존하며 원본 DWORD의 낮은 바이트부터 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 물리다.</param>
    /// <param name="offset">쓸 바이트 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        if (offset < 0 || offset + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "Player 식별자 쓰기 범위 오류");
        }

        // 원본 DWORD의 낮은 바이트부터 쓰는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>NUL 없는 원본 버퍼 초과를 거부하며 첫 NUL까지 Latin1 문자열로 읽는다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="offset">읽을 바이트 위치다.</param>
    /// <param name="capacity">읽을 폭이다.</param>
    private static string Text(byte[] bytes, int offset, int capacity)
    {
        if (offset < 0 || capacity < 0 || offset + capacity > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "Player 식별자 문자열 범위 오류");
        }

        int end = Array.IndexOf(bytes, (byte)0, offset, capacity);
        if (end < 0)
        {
            throw new InvalidOperationException("Player 식별자 NUL 없는 문자열");
        }

        return Encoding.Latin1.GetString(bytes, offset, end - offset);
    }

    /// <summary>원본 strcpy처럼 최대 40바이트 이름과 종결자만 쓴다. 40바이트 이름의 NUL은 인접 주소 첫 바이트이며 기존 60바이트 물리 안이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="record">쓸 60바이트 레코드다.</param>
    /// <param name="name">쓸 이름이다. 첫 NUL까지만 본다.</param>
    private static void SetName(byte[] record, string name)
    {
        int nul = name.IndexOf('\0');
        if (nul >= 0)
        {
            name = name.Substring(0, nul);
        }

        byte[] bytes = Encoding.Latin1.GetBytes(name);
        if (bytes.Length > PlayerIdentityValues.NameBytes)
        {
            throw new InvalidOperationException("Player 식별자 40바이트 이름 초과");
        }

        // 이름 뒤의 원문 표식을 지우지 않고 복사하는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
        for (int index = 0; index < bytes.Length; ++index)
        {
            record[PlayerIdentityValues.NameOffset + index] = bytes[index];
        }

        record[PlayerIdentityValues.NameOffset + bytes.Length] = 0;
    }

    /// <summary>원본 C locale의 ASCII 알파벳만 접으며 확장 바이트는 같은 값끼리만 일치한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="first">비교할 첫 문자열이다.</param>
    /// <param name="second">비교할 둘째 문자열이다.</param>
    private static bool Equal(string first, string second)
    {
        if (first.Length != second.Length)
        {
            return false;
        }

        // 각 원문 바이트를 같은 규칙으로 비교하는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
        for (int index = 0; index < first.Length; ++index)
        {
            char left = first[index] >= 'A' && first[index] <= 'Z' ? (char)(first[index] + ('a' - 'A')) : first[index];
            char right = second[index] >= 'A' && second[index] <= 'Z' ? (char)(second[index] + ('a' - 'A')) : second[index];
            if (left != right)
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>PATCH00484190/CD004015e0처럼 원본이 쓴 필드만 초기화하며 이름 뒤와 마지막 DWORD는 보존한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="record">쓸 60바이트 레코드다.</param>
    /// <param name="mode">쓸 모드다.</param>
    public static void Initialize(byte[] record, uint mode)
    {
        ArgumentNullException.ThrowIfNull(record);
        if (record.Length != PlayerIdentityValues.RecordBytes)
        {
            throw new ArgumentException("Player 식별자 60바이트 레코드 필요", nameof(record));
        }

        Put(record, 0, 0u);
        Put(record, 4, 0u);
        record[PlayerIdentityValues.NameOffset] = 0;
        Put(record, PlayerIdentityValues.AddressOffset, 0u);
        Put(record, PlayerIdentityValues.ModeOffset, mode);
    }

    /// <summary>현재 로컬→battle/editor Player 1..8→metamap 40→채팅 100 순서로 실제 레코드를 갱신한다. 반환 true는 모든 필드가 완성됐다는 뜻이 아니다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="record">갱신할 60바이트 레코드다. 호출자가 소유한다.</param>
    /// <returns>부분완료 관문을 만족하면 참이다.</returns>
    public bool Resolve(byte[] record)
    {
        ArgumentNullException.ThrowIfNull(record);
        if (record.Length != PlayerIdentityValues.RecordBytes)
        {
            throw new ArgumentException("Player 식별자 60바이트 레코드 필요", nameof(record));
        }

        if ((Read(record, 0) != 0u && Read(record, 0) == Need(_reads.LocalOwner)())
            || (Read(record, 4) != 0u && Read(record, 4) == Need(_reads.Subscriber)())
            || (record[PlayerIdentityValues.NameOffset] != 0
                && Equal(Text(record, PlayerIdentityValues.NameOffset, PlayerIdentityValues.TerminatedNameBytes), Need(_reads.Name)()))
            || (Read(record, PlayerIdentityValues.AddressOffset) != 0u
                && Read(record, PlayerIdentityValues.AddressOffset) == Need(_reads.Address)()))
        {
            SetName(record, Need(_reads.Name)());
            Put(record, PlayerIdentityValues.AddressOffset, Need(_reads.Address)());
            Put(record, 4, Need(_reads.Subscriber)());
            Put(record, 0, Need(_reads.LocalOwner)());
            return true;
        }

        bool patch = _edition == OriginalEdition.Patch1078;
        int playerStride = patch ? 180 : 172;
        int metaStride = patch ? 460 : 296;
        if (Need(_reads.Battle)() || Need(_reads.Editor)())
        {
            // Player 0은 검색하지 않고 실제 1..8의 첫 OR 일치를 쓰는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
            for (uint owner = 1u; owner < 9u; ++owner)
            {
                int start = (int)(owner * (uint)playerStride);
                uint subscriber = Read(_players, start + 0x2c);
                string name = Text(_players, start + 0xc, 32);
                if ((Read(record, 0) != 0u && Read(record, 0) == owner)
                    || (Read(record, 4) != 0u && Read(record, 4) == subscriber)
                    || (record[PlayerIdentityValues.NameOffset] != 0
                        && Equal(Text(record, PlayerIdentityValues.NameOffset, PlayerIdentityValues.TerminatedNameBytes), name)))
                {
                    Put(record, 0, owner);
                    Put(record, 4, subscriber);
                    SetName(record, name);
                    Put(record, PlayerIdentityValues.AddressOffset, 0u);
                    break;
                }
            }

            if (Read(record, 0) != 0u && Read(record, 4) != 0u && record[PlayerIdentityValues.NameOffset] != 0)
            {
                return true;
            }
        }

        if (Need(_reads.Metamap)())
        {
            byte[] meta = MetaRecords();
            if (meta.Length == 0)
            {
                throw new InvalidOperationException("실제 metamap 식별자 기록 미연결");
            }

            // 실제 40개의 활성 기록만 구독자 또는 이름으로 비교하는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
            for (int index = 0; index < PlayerIdentityValues.MetaCount; ++index)
            {
                int start = index * metaStride;
                bool active = patch ? (meta[start + 100] & 1) != 0 : (Read(meta, start + 72) & 1u) != 0u;
                int nameOffset = patch ? 38 : 8;
                if (active
                    && ((Read(record, 4) != 0u && Read(record, 4) == Read(meta, start + 4))
                        || (record[PlayerIdentityValues.NameOffset] != 0
                            && Equal(Text(record, PlayerIdentityValues.NameOffset, PlayerIdentityValues.TerminatedNameBytes), Text(meta, start + nameOffset, 40)))))
                {
                    Put(record, 0, Read(meta, start));
                    Put(record, 4, Read(meta, start + 4));
                    SetName(record, Text(meta, start + nameOffset, 40));
                    Put(record, PlayerIdentityValues.AddressOffset, 0u);
                    break;
                }
            }
        }

        if (Read(record, 4) != 0u && record[PlayerIdentityValues.NameOffset] != 0)
        {
            return true;
        }

        // 채팅은 100개 전체의 첫 일치이며 활성 flag나 구독자 비어 있음 필터를 추가하지 않는 반복문이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.
        for (int index = 0; index < PlayerIdentityValues.ChatCount; ++index)
        {
            int start = index * PlayerIdentityValues.ChatStride;
            uint subscriber = Read(_chat, start + 44);
            string name = Text(_chat, start, PlayerIdentityValues.TerminatedNameBytes);
            if ((Read(record, 4) != 0u && Read(record, 4) == subscriber)
                || (record[PlayerIdentityValues.NameOffset] != 0
                    && Equal(Text(record, PlayerIdentityValues.NameOffset, PlayerIdentityValues.TerminatedNameBytes), name)))
            {
                Put(record, 0, 0u);
                Put(record, 4, subscriber);
                SetName(record, name);
                break;
            }
        }

        return Read(record, 4) != 0u && record[PlayerIdentityValues.NameOffset] != 0;
    }

    /// <summary>PATCH004844f0/CD004019e0처럼 소유자 번호를 받아 실제 구독자 DWORD를 반환한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="owner">찾을 소유자 번호다.</param>
    public uint SubscriberForOwner(uint owner)
    {
        byte[] record = new byte[PlayerIdentityValues.RecordBytes];
        Initialize(record, 2u);
        Put(record, 0, owner);
        Resolve(record);
        return Read(record, 4);
    }

    /// <summary>PATCH00484550/CD00401ad0처럼 구독자를 받아 소유자 번호를 반환한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="subscriber">찾을 구독자 번호다.</param>
    public uint OwnerForSubscriber(uint subscriber)
    {
        byte[] record = new byte[PlayerIdentityValues.RecordBytes];
        Initialize(record, 2u);
        Put(record, 4, subscriber);
        Resolve(record);
        return Read(record, 0);
    }

    /// <summary>PATCH00484520/CD00401aa0처럼 구독자를 받아 주소 DWORD를 반환한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="subscriber">찾을 구독자 번호다.</param>
    public uint AddressForSubscriber(uint subscriber)
    {
        byte[] record = new byte[PlayerIdentityValues.RecordBytes];
        Initialize(record, 2u);
        Put(record, 4, subscriber);
        Resolve(record);
        return Read(record, PlayerIdentityValues.AddressOffset);
    }

    /// <summary>PATCH00484410/CD00401890처럼 구독자의 이름이며 조회 실패 시 현재 대체 이름을 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="subscriber">찾을 구독자 번호다.</param>
    public string NameForSubscriber(uint subscriber)
    {
        byte[] record = new byte[PlayerIdentityValues.RecordBytes];
        Initialize(record, 0u);
        Put(record, 4, subscriber);
        return Resolve(record)
            ? Text(record, PlayerIdentityValues.NameOffset, PlayerIdentityValues.TerminatedNameBytes)
            : Need(_reads.UnknownName)();
    }

    /// <summary>PATCH00484480/CD00401920처럼 모드 0의 소유자 입력으로 같은 Resolve를 실행하고 실패 시 현재 대체 이름을 쓴다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    /// <param name="owner">찾을 소유자 번호다.</param>
    public string NameForOwner(uint owner)
    {
        byte[] record = new byte[PlayerIdentityValues.RecordBytes];
        Initialize(record, 0u);
        Put(record, 0, owner);
        return Resolve(record)
            ? Text(record, PlayerIdentityValues.NameOffset, PlayerIdentityValues.TerminatedNameBytes)
            : Need(_reads.UnknownName)();
    }

    /// <summary>실제 조회 몸체와 같은 원본 판본이다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public OriginalEdition Edition() => _edition;

    /// <summary>호출자가 같은 현재 9 Player를 사용하게 한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public byte[] PlayerRecords() => _players;

    /// <summary>현재 조회가 실제로 도달한 때만 읽는다. 고정 저장소의 기존 독립 계약도 유지한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public byte[] MetaRecords()
    {
        byte[] bytes = _reads.MetaRecords is not null ? _reads.MetaRecords() : _meta;
        int stride = _edition == OriginalEdition.Patch1078 ? 460 : 296;
        if (bytes.Length != 0 && bytes.Length != PlayerIdentityValues.MetaCount * stride)
        {
            throw new ArgumentException("현재 Player 식별자 메타 40기록 물리 크기 오류", nameof(bytes));
        }

        return bytes;
    }

    /// <summary>현재 모드/로컬 식별자 조회를 다른 실제 소비자와 공유한다. 2026-11-11 F3 Player 식별자 본체 이식에서 추가됐다.</summary>
    public PlayerIdentityReads CurrentInputs() => _reads;
}
