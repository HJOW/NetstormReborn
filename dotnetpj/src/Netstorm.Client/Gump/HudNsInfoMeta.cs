// 메타 NSInfo의 실제 참가자 등록 명부 정렬·행 작성을 같은 CS/Gump 저장소에 수행한다.
// cpppj/src/client/HudNsInfoMeta.h·cpp 의 HudNsInfoMetaGlobals/HudNsInfoMetaHooks/RefreshNsInfoRoster(0047e630) 를 이식한다.
// 0.906에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using System.Text;

namespace Netstorm.Client.Gump;

// 메타 NSInfo 모듈을 담는 네임스페이스다. 0.906에서 추가됐다.

// 두 원문 몸체가 현재 위치에서 읽는 실제 전역이다. 0.906에서 추가됐다.
public sealed class HudNsInfoMetaGlobals
{
    /// <summary>같은 CS 참가자40·전투10 기록을 소유한다. 0.906에서 추가됐다.</summary>
    public required ClientChallengeGlobals Challenge { get; init; }

    /// <summary>같은 005930b0 정렬 표/현재 NSInfo를 소유한다. 0.906에서 추가됐다.</summary>
    public required ClientNsInfoGlobals Info { get; init; }

    /// <summary>같은 594fc8의 현재 메타맵 모드를 빌린다. 0.906에서 추가됐다.</summary>
    public required Func<uint> Metamap { get; init; }

    /// <summary>같은 595344의 현재 플레이어 인식 값을 빌린다. 0.906에서 추가됐다.</summary>
    public required Func<uint> Known { get; init; }

    /// <summary>같은 540c70의 현재 로컬 소유자를 빌린다. 0.906에서 추가됐다.</summary>
    public required Func<uint> Owner { get; init; }
}

// 실제 UI 처리기·현재 색/기술 값만 선택된 위치에서 호출한다. 0.906에서 추가됐다.
public sealed class HudNsInfoMetaHooks
{
    /// <summary>같은 0xd8 NSInfo 노드다. 0.906에서 추가됐다.</summary>
    public Func<uint, HudNsInfoNode?>? Root { get; set; }

    /// <summary>004bc250 모드0의 실제 문자열 처리다. 0.906에서 추가됐다.</summary>
    public Action<uint, string>? Replace { get; set; }

    /// <summary>004ba210의 같은 버퍼 noWrap 쓰기다. 0.906에서 추가됐다.</summary>
    public Action<uint, uint>? Wrap { get; set; }

    /// <summary>현재 팔레트색/metadataMode의 원문 DWORD를 읽는다. 0.906에서 추가됐다.</summary>
    public Func<uint, uint>? Read { get; set; }

    /// <summary>같은 Pool의 실제 SID+22 BYTE다. 0.906에서 추가됐다.</summary>
    public Func<uint, uint>? ObjectOwner { get; set; }

    /// <summary>0042b210의 현재 TYPE 기술 합계다. 0.906에서 추가됐다.</summary>
    public Func<ReadOnlyMemory<byte>, uint>? Weight { get; set; }

    /// <summary>004ba0f0의 길이-1/실제 정렬행 metadata다. 0.906에서 추가됐다.</summary>
    public Action<uint, string, uint[], uint>? Append { get; set; }
}

// 0047e630 전체의 39명 초기 순서·원문 쌍교환 정렬·이름/문맥/행 metadata다. 0.906에서 추가됐다.
public static class HudNsInfoMetaRoster
{
    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.906에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("NSInfo 메타의 실제 선택 장치 미연결");

    /// <summary>지정한 실제 폭만 읽고 원문 필드 밖 접근은 그 위치에서 거부한다. 0.906에서 추가됐다.</summary>
    private static uint ReadField(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 낮은 BYTE부터 원문 폭만 조립한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "NSInfo 메타의 실제 CS 필드 부족");
        }

        uint value = 0;
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>현재 레코드의 실제 첫 NUL을 찾으며 임의 길이로 이름을 잘라 성공시키지 않는다. 0.906에서 추가됐다.</summary>
    private static string ParticipantName(ReadOnlySpan<byte> record)
    {
        ReadOnlySpan<byte> text = record.Slice(0x26);
        int end = text.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "NSInfo 이름의 실제 NUL 없음");
        }

        return Encoding.Latin1.GetString(text[..end]);
    }

    /// <summary>원문은 정렬되지 않은 유효표를 표준 sort가 아닌 전체 39×39 쌍교환으로 재배치한다. 0.906에서 추가됐다.</summary>
    /// <param name="token">같은 0xd8 NSInfo 노드 토큰이다.</param>
    /// <param name="globals">두 원문 몸체가 현재 위치에서 읽는 실제 전역이다.</param>
    /// <param name="hooks">실제 UI 처리기·현재 색/기술 값이다.</param>
    public static void Refresh(uint token, HudNsInfoMetaGlobals globals, HudNsInfoMetaHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(globals.Challenge);
        ArgumentNullException.ThrowIfNull(globals.Info);
        if (Need(globals.Metamap)() == 0 || Need(globals.Known)() == 0)
        {
            return;
        }

        uint shade = Need(hooks.Read)(0x5c78d0u);
        uint yellow = Need(hooks.Read)(0x59afd8u);
        uint[] ownContext = [2, 0, yellow, 0, 0, shade, 1, 1];
        HudNsInfoNode root = Need(hooks.Root)(token) ?? throw new InvalidOperationException("NSInfo 메타의 실제 Root 없음");
        Need(hooks.Replace)(root.Roster, "");
        Need(hooks.Wrap)(root.Roster, 0);
        // 0번을 보존하며 005930b4..314f의 39DWORD만 초기 순서로 쓴다.
        for (uint index = 1; index < 40; index++)
        {
            globals.Info.RosterOrder[index] = index;
        }

        // 바깥 현재 후보는 교환 직후 새 참가자로 재조회한다.
        for (uint outer = 1; outer < 40; outer++)
        {
            ReadOnlyMemory<byte> current = globals.Challenge.Participant(globals.Info.RosterOrder[outer]);
            if ((ReadField(current.Span, 0x64, 1) & 1u) == 0)
            {
                continue;
            }

            // 유효한 상대의 레벨 BYTE를 먼저 비교하고 동률 때만 상대→현재 순서로 기술 합계를 읽는다.
            for (uint inner = 1; inner < 40; inner++)
            {
                ReadOnlyMemory<byte> other = globals.Challenge.Participant(globals.Info.RosterOrder[inner]);
                if ((ReadField(other.Span, 0x64, 1) & 1u) == 0)
                {
                    continue;
                }

                uint level = ReadField(current.Span, 0x58, 1);
                uint otherLevel = ReadField(other.Span, 0x58, 1);
                bool exchange = level < otherLevel;
                if (level == otherLevel)
                {
                    uint otherWeight = Need(hooks.Weight)(other.Slice(0x54, 10));
                    uint currentWeight = Need(hooks.Weight)(current.Slice(0x54, 10));
                    exchange = currentWeight < otherWeight;
                }

                if (exchange)
                {
                    (globals.Info.RosterOrder[outer], globals.Info.RosterOrder[inner]) = (globals.Info.RosterOrder[inner], globals.Info.RosterOrder[outer]);
                    current = globals.Challenge.Participant(globals.Info.RosterOrder[outer]);
                }
            }
        }

        // 유효행만 추가하되 metadata는 실제 참가자 번호가 아닌 정렬 배열의 1..39행 번호다.
        for (uint row = 1; row < 40; row++)
        {
            ReadOnlyMemory<byte> record = globals.Challenge.Participant(globals.Info.RosterOrder[row]);
            if ((ReadField(record.Span, 0x64, 1) & 1u) == 0)
            {
                continue;
            }

            bool own = Need(hooks.ObjectOwner)(ReadField(record.Span, 0xb8)) == Need(globals.Owner)();
            uint rank = Need(hooks.Weight)(record.Slice(0x54, 10));
            string text = $" {ReadField(record.Span, 0x58, 1) + 1u}.{rank} " + ParticipantName(record.Span) + '\n';
            if (Encoding.Latin1.GetByteCount(text) >= 120)
            {
                throw new ArgumentOutOfRangeException(nameof(record), "NSInfo 참가자행의 실제120BYTE 범위");
            }

            uint minor = ReadField(record.Span, 0xb2, 2);
            uint[] context = [0, 0, minor == 78 ? 0xc4u : 0xdfu, 0, 0, 0, 1, 1];
            if (minor > 78)
            {
                context = [0, 0, 0x9du, 0, 0, 0, 1, 1];
            }

            context[0] |= 2u;
            context[5] = Need(hooks.Read)(0x5c78d0u);
            context[6] = 1;
            context[7] = 1;
            uint[] selected = own && Need(hooks.Read)(0x5c85c8u) != 0 ? ownContext : context;
            Need(hooks.Append)(root.Roster, text, selected, row);
        }
    }
}
