using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Mission;

/// <summary>기본 디스크 열거와 실제 Fort 섹션에서 편집 맵 목록의 메타데이터를 읽는다.</summary>
public sealed class SoloMapCatalog
{
    private readonly ConfigInterface _config;
    private readonly Func<string> _user;
    private readonly Func<string, string> _translate;
    private readonly Func<byte[]> _uninitializedState;
    /// <summary>패턴의 파일 목록을 돌려주는 열거 장치다. 편집 작업본 장치를 주지 않으면 기본 디스크만 OS 순서로 열거한다.</summary>
    private readonly Func<string, IReadOnlyList<string>> _enumerate;
    /// <summary>분류와 메타마다 현재 파일을 읽는 장치다. 편집 작업본 장치를 주지 않으면 기본 파일 시스템의 우선순위로 읽는다.</summary>
    private readonly Func<string, byte[]?> _read;
    private Probe? _current;
    private string _fileSpec = "fortSpec";
    /// <summary>같은 Client 수명의 50칸 캐시다.</summary>
    public SoloMapListState State { get; } = new();
    /// <summary>같은 실제 캐시와 파일 장치를 사용하는 원본 목록 본체다.</summary>
    public RawSoloMapList List { get; }

    /// <summary>
    /// 현재 파일·설정·사용자와 빈 State의 원본 미정 바이트 공급자를 빌린다. <paramref name="enumerate"/>와 <paramref name="read"/>를 주면
    /// 목록 열거와 파일 읽기가 기본 디스크 대신 편집 작업본 장치(<c>EditorCatalogWorkspace.Enumerate</c>/<c>Read</c>)를 거친다.
    /// (cpppj SoloMapCatalogEffects의 enumeratePaths·readFile에 해당하며 2026-11-01 0.126에서 추가)
    /// </summary>
    public SoloMapCatalog(GameFileSystem files, ConfigInterface config, Func<string> user, Func<string, string> translate, Func<byte[]> uninitializedState,
        Func<string, IReadOnlyList<string>>? enumerate = null, Func<string, byte[]?>? read = null)
    {
        _config = config; _user = user; _translate = translate; _uninitializedState = uninitializedState;
        _enumerate = enumerate ?? files.MatchDisk; _read = read ?? files.TryReadAllBytes;
        List = new RawSoloMapList(State, new SoloMapListHooks
        {
            MayEditMissions = () => _config.GetInt("mayEditMissions"),
            Enumerate = () => _enumerate(_config.PathSpec(_fileSpec, "*")).Select(MapName).ToArray(),
            Kind = Kind,
            Construct = () => _current = new Probe(_uninitializedState),
            Load = name => Current().Load(_read(_config.PathSpec(_fileSpec, name))),
            Present = () => Current().Present,
            Subscriber = () => Current().Subscriber(), StateByte = () => Current().State()[0],
            UserName = () => _user(), Reserved = () => Current().Present ? 0u : throw new InvalidOperationException("단독 맵 예약 조회의 누락 Template"),
            Translate = text => _translate(text)
        });
    }

    private Probe Current() => _current ?? throw new InvalidOperationException("단독 맵 목록의 임시 Template 생성 누락");
    private static string MapName(string path)
    {
        string normalized = path.Replace('\\', '/'); int slash = normalized.LastIndexOf('/'); string name = normalized[(slash + 1)..];
        int dot = name.IndexOf('.'); return dot < 0 ? name : name[..dot];
    }
    /// <summary>동일 파일 키 버퍼의 현재 내용을 단독/일반 명세로 바꾼다.</summary>
    public void SelectFileSpec(bool solo) => _fileSpec = solo ? "SOLOSpec" : "fortSpec";
    /// <summary>다음 목록 조회에서 캐시를 다시 만든다.</summary>
    public void Invalidate() => State.Dirty = 1;
    /// <summary>분류는 별도의 임시 Template으로 실제 파일을 읽는다.</summary>
    public byte Kind(string name) { var probe = new Probe(_uninitializedState); probe.Load(_read(_config.PathSpec(_fileSpec, name))); return probe.State()[17]; }

    /// <summary>파일별 임시 Template의 Subscriber·State를 소유하며 잘못된 입력은 거부한다.</summary>
    private sealed class Probe(Func<byte[]> uninitialized)
    {
        private byte[][]? _sections;
        public bool Present => _sections != null;
        public void Load(byte[]? image)
        {
            if (image == null) return;
            if (image.Length < 2 || image[0] != 0x46) throw new InvalidDataException("Template 파일 서명 오류");
            var sections = new byte[35][]; int offset = 2;
            for (int i = 0; i < sections.Length; i++)
            {
                if (offset > image.Length - 2) throw new InvalidDataException("Template 섹션 길이 누락");
                int length = BinaryPrimitives.ReadInt16LittleEndian(image.AsSpan(offset));
                if (length < 2 || length > image.Length - offset) throw new InvalidDataException("Template 섹션 물리 범위 오류");
                sections[i] = image.AsSpan(offset + 2, length - 2).ToArray(); offset += length;
            }
            _sections = sections;
        }
        public byte[] State()
        {
            if (_sections == null) throw new InvalidOperationException("Player State의 실제 Template 없음");
            byte[] state = _sections[1];
            if (state.Length == 0)
            {
                state = uninitialized(); if (state.Length != 28) throw new InvalidOperationException("빈 State의 실제 미정 바이트 28개 필요");
                state[0] = (byte)((state[0] & 0xf9) | 1); Array.Clear(state, 4, 6); Array.Clear(state, 12, 6); Array.Clear(state, 22, 4);
            }
            else if (state.Length < 28) Array.Resize(ref state, 28);
            _sections[1] = state; return state;
        }
        public string Subscriber()
        {
            if (_sections == null) throw new InvalidOperationException("Subscriber의 실제 Template 없음"); byte[] bytes = _sections[0];
            if (bytes.Length < 6) throw new InvalidDataException("Subscriber 길이 부족"); int length = BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(4));
            if (length >= 256 || length > bytes.Length - 6) throw new InvalidDataException("Subscriber 이름 범위 오류");
            string name = Encoding.Latin1.GetString(bytes, 6, length); int nul = name.IndexOf('\0'); return nul < 0 ? name : name[..nul];
        }
    }
}
