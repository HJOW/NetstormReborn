using Netstorm.Core.Original;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Client.Host;

public sealed partial class CampaignTemplateStorage
{
    /// <summary>
    /// 하나의 manager를 빌린 여러 헤더가 공유하는 실제 이미지다. 파싱한 요새(<see cref="Fort"/>)·원본 276바이트 manager·60바이트 섹션 노드 표·
    /// 같은 State/지형/Money/Badges/Technology 커서를 한 객체가 평생 소유한다. cpppj <c>CampaignTemplateStorage::Image</c>를 옮겼다.
    /// 커서는 교체하지 않고 필드만 바꾸므로(C++의 참조 반환과 같은 의미) 저장소가 돌려준 참조가 오래 유효하다.
    /// </summary>
    /// <remarks>
    /// 2026-11-01 0.131(TS1)에서 노드·버퍼·<see cref="SyncStreams"/>·커서 공개·선택·소멸을 옮겼다. 동적 섹션 생성·쓰기·이름 교체·State 최소 확장은
    /// 0.132(TS2)에서 <c>CampaignTemplateStorage.ImageSections.cs</c>로, 파일 읽기/저장은 0.133(TS3)에서 <c>CampaignTemplateStorage.ImageFile.cs</c>로,
    /// 빈 Image 생성자와 로그인 이미지용 복제·덤프·이미지 작성은 0.134(TS4)에서 이 파일과 <c>CampaignTemplateStorage.ImageLogin.cs</c>로 같은 partial 클래스에 더했다
    /// (계획서 <c>docs/dotnet-template-storage-plan-20261101.md</c> 4절).
    /// </remarks>
    private sealed partial class Image
    {
        /// <summary>원본 60바이트 섹션 노드 하나다. 첫 16바이트 MemStream·이름 40바이트·next DWORD를 같은 노드에 둔다.</summary>
        internal sealed class Stream
        {
            /// <summary>원본 60바이트 노드 물리다(+0 data 토큰, +4 pos, +8 end, +c alloc, +10 이름 40바이트, +38 next).</summary>
            internal byte[] Bytes { get; } = new byte[60];

            /// <summary>현재 실제 버퍼가 놓인 Fort 35칸의 위치다. 노드 핸들과 독립이며 비표준 이름(Temp 등)은 null이다.</summary>
            internal int? Canonical { get; set; }

            /// <summary>비표준 이름 노드의 실제 할당이다. Rename 때 같은 할당을 넘긴다.</summary>
            internal RawSectionBuffer Extra { get; } = new();

            /// <summary>같은 이름 재생성·노드 반납과 구별하는 생성 당시의 비영 수명이다.</summary>
            internal ulong Generation { get; set; }

            /// <summary>이 노드의 raw 커서·버퍼를 인라인 쓰기에 빌려준 동안만 참이다.</summary>
            internal bool Accessing { get; set; }
        }

        /// <summary>manager 하나의 실제 폭(10.78 Template manager)이다.</summary>
        private const int ManagerBytes = 276;

        /// <summary>manager 안 현재 파일명 배열의 시작 위치와 폭이다(NUL 포함).</summary>
        private const int ManagerNameOffset = 12;

        /// <summary>manager 안 현재 파일명 배열의 실제 폭이다.</summary>
        private const int ManagerNameBytes = 256;

        /// <summary>노드 안 이름의 시작 위치다.</summary>
        private const int NodeNameOffset = 16;

        /// <summary>노드 안 이름의 실제 폭이다.</summary>
        private const int NodeNameBytes = 40;

        /// <summary>노드 안 next DWORD의 위치다.</summary>
        private const int NodeNext = 0x38;

        /// <summary>같은 manager 안에서 재생성마다 증가하는 비영 번호다.</summary>
        private ulong _nextStreamGeneration;

        /// <summary>이름·표준 파일 위치와 독립적인 단조 증가 노드 토큰이다.</summary>
        private uint _nextStreamHandle = 1;

        /// <summary>외부 파싱 자료의 각 표준 위치를 최초 목록에 한 번만 연결했는지 나타낸다.</summary>
        private readonly bool[] _seeded = new bool[FortTemplate.SectionCount];

        /// <summary>Client와 같은 lazy 이름표이며 이 Image보다 긴 Storage owner에서 빌린다.</summary>
        private readonly ClientTemplateSectionNames _sectionNames;

        /// <summary>공개한 Money/Badges/Technology 커서의 유효 비트 1/2/4다. 미공개 노드의 초기 상태는 보존한다.</summary>
        private uint _loadCursors;

        /// <summary>현재 typed 커서를 소유한 실제 노드다. Rename/재생성 뒤 이름으로 다시 공개한다.</summary>
        private uint _stateHandle, _territoryHandle, _moneyHandle, _badgesHandle, _technologyHandle;

        /// <summary>실제 manager 원본 바이트(vptr, head/current, 파일명 256, 서명/FE 판정 DWORD)다.</summary>
        internal byte[] Manager { get; } = new byte[ManagerBytes];

        /// <summary>노드 핸들 오름차순의 실제 노드 표다. 순회 순서가 <see cref="SyncStreams"/>와 <see cref="LoadCursor"/>의 결과를 정한다.</summary>
        internal SortedDictionary<uint, Stream> Streams { get; } = [];

        /// <summary>원본 추가/교체 뒤 목록 순서를 고정 파일 섹션 번호와 구별한다(원본 연결 순서의 보조 기록).</summary>
        internal List<uint> StreamOrder { get; } = [];

        /// <summary>원문 생성자까지 마쳤는가.</summary>
        internal bool Constructed { get; private set; }

        /// <summary>파싱 자료와 원시 State 섹션을 포함하는 실제 요새다. 소멸 때 새 빈 객체로 바뀐다.</summary>
        internal FortTemplate Fort { get; set; }

        /// <summary>같은 <see cref="Fort"/> State 섹션의 현재 커서다.</summary>
        internal PlayerTemplateStateStream State { get; } = new();

        /// <summary>같은 Territory 섹션의 현재 커서다.</summary>
        internal FortTerritoryStream Territory { get; } = new();

        /// <summary>같은 Money 노드의 현재 pos/end다. SP 값의 편의 사본을 쓰지 않는다.</summary>
        internal FortMoneyCursor Money { get; } = new();

        /// <summary>같은 Badges 섹션의 버퍼와 present/pos/end다.</summary>
        internal FortMetaStream Badges { get; } = new();

        /// <summary>같은 Technology 섹션의 버퍼와 present/pos/end다.</summary>
        internal FortMetaStream Technology { get; } = new();

        /// <summary>manager+c의 현재 이름을 기존 문자열 API에 빌려주는 읽기 투영이다. 변경의 원본은 manager 물리다.</summary>
        internal string Name { get; set; }

        /// <summary>State 확장 중 Get의 원문 복사가 끝날 때까지만 이전 할당을 보유한다.</summary>
        internal RawSectionBuffer RetiredState { get; } = new();

        /// <summary>
        /// 파싱 자료를 받아 원문 생성자(공유 version 0 쓰기)를 실행하고, 파일 입력이 아니면 State·Territory 커서를 같은 섹션 버퍼에 연결한 뒤 첫 목록을 만든다.
        /// 파일 입력이면 원본 읽기 부모가 초기 목록을 만든다.
        /// </summary>
        /// <param name="value">요새. 이 Image가 소유권을 넘겨받는다(호출자가 계속 쓰려면 <see cref="FortTemplate.Clone"/>한 사본을 넘긴다).</param>
        /// <param name="file">파일 이름(255바이트 이하, NUL 없음).</param>
        /// <param name="globals">같은 Client의 직렬화 버전 전역.</param>
        /// <param name="names">같은 Client의 이름표 owner.</param>
        /// <param name="fileImage">참이면 State·Territory 연결과 첫 목록 생성을 건너뛴다.</param>
        /// <exception cref="ArgumentException">파일 이름이 256바이트 이상이거나 NUL을 포함한다.</exception>
        internal Image(FortTemplate value, string file, FortSerializationGlobals globals, ClientTemplateSectionNames names, bool fileImage = false)
        {
            ArgumentNullException.ThrowIfNull(value);
            ArgumentNullException.ThrowIfNull(file);
            ArgumentNullException.ThrowIfNull(globals);
            ArgumentNullException.ThrowIfNull(names);
            _sectionNames = names;
            Fort = value;
            Name = file;
            if (Name.Length >= ManagerNameBytes || Name.Contains('\0'))
            {
                throw new ArgumentException("Template 실제 파일명256BYTE 계약 위반");
            }
            // 실제 DWORD 객체의 BYTE 뷰에 해당하는 임시 4바이트에 0을 쓰는 효과를 원본 생성자에서 수행한 뒤 같은 전역에 돌려준다
            Span<byte> global = stackalloc byte[4];
            Put(global, 0, globals.Version);
            RawTemplateLoad.ConstructManager(Manager, Name, global);
            globals.Version = Read(global, 0);
            Constructed = true;
            if (fileImage)
            {
                return;
            }
            RawSectionBuffer stateBytes = SectionOrEmpty(StateSection());
            State.Bytes = stateBytes.Memory;
            State.Position = 0;
            State.End = (uint)stateBytes.Count;
            State.Present = stateBytes.Count != 0;
            RawSectionBuffer territoryBytes = SectionOrEmpty(TerritorySection());
            Territory.Set(territoryBytes.Memory, 0, (uint)territoryBytes.Count);
            SyncStreams();
        }

        /// <summary>
        /// 원문 <c>new 276</c>의 같은 할당이다. 원문 생성자와 목록 생성은 아직 하지 않고, metadata의 선행 노드 투영도 막기 위해 모든 표준 위치를 연결된 것으로 표시한다.
        /// 이름과 직렬화 버전은 <see cref="ConstructName"/>이 나중에 쓴다.
        /// </summary>
        /// <param name="names">같은 Client의 이름표 owner.</param>
        /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
        internal Image(ClientTemplateSectionNames names)
        {
            ArgumentNullException.ThrowIfNull(names);
            _sectionNames = names;
            Fort = new FortTemplate();
            Name = "";
            Array.Fill(_seeded, true);
        }

        /// <summary>
        /// 원문 생성자를 같은 생성 수명의 실제 manager에 실행한다: 공유 Client version을 0으로 쓰고 이름과 기본 서명/FE 판정을 기록한다. 이름표 할당은 하지 않는다.
        /// </summary>
        /// <param name="file">파일 이름(255바이트 이하, NUL 없음).</param>
        /// <param name="globals">같은 Client의 직렬화 버전 전역.</param>
        /// <exception cref="ArgumentException">이미 생성했거나 노드가 있거나 이름이 256바이트 이상이거나 NUL을 포함한다.</exception>
        /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
        internal void ConstructName(string file, FortSerializationGlobals globals)
        {
            ArgumentNullException.ThrowIfNull(file);
            ArgumentNullException.ThrowIfNull(globals);
            if (Constructed || Streams.Count != 0 || file.Length >= ManagerNameBytes || file.Contains('\0'))
            {
                throw new ArgumentException("Template 빈Image 실제ctor계약");
            }
            // 실제 DWORD 객체의 BYTE 뷰에 해당하는 임시 4바이트에 0을 쓰는 효과를 원본 생성자에서 수행한 뒤 같은 전역에 돌려준다
            Span<byte> global = stackalloc byte[4];
            Put(global, 0, globals.Version);
            RawTemplateLoad.ConstructManager(Manager, file, global);
            globals.Version = Read(global, 0);
            Name = file;
            Constructed = true;
        }

        /// <summary>
        /// 다음 노드 핸들이다. 서로 다른 manager의 노드를 같은 복제 장치에서 식별하려고 대상이 원본의 값을 이어받는다(원문 노드 범위 뒤에서 시작하게 한다).
        /// </summary>
        /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
        internal uint NextStreamHandle
        {
            get => _nextStreamHandle;
            set => _nextStreamHandle = value;
        }

        /// <summary>표준 위치의 섹션을 돌려주고 파싱 자료에 그 위치가 없으면 빈 버퍼를 돌려준다(목록을 늘리지 않는다).</summary>
        /// <param name="index">표준 위치.</param>
        private RawSectionBuffer SectionOrEmpty(int index)
        {
            return index < Fort.Sections.Count ? Fort.Sections[index] : new RawSectionBuffer();
        }

        /// <summary>State의 실제 파일 섹션 위치를 이름표에서 찾는다.</summary>
        internal static int StateSection() => NamedSection("State");

        /// <summary>지형 getter도 파서의 편의 사본 대신 같은 원시 섹션 버퍼를 조회하기 위한 Territory의 파일 섹션 위치다.</summary>
        internal static int TerritorySection() => NamedSection("Territory");

        /// <summary>이름표에서 정확히 일치하는 표준 위치를 찾는다. 이름표에 없으면 구현 오류다.</summary>
        /// <param name="name">섹션 이름.</param>
        private static int NamedSection(string name)
        {
            IReadOnlyList<string> names = FortTemplate.SectionNames;
            // 이름표를 처음부터 훑어 위치를 찾는다
            for (int index = 0; index < names.Count; index++)
            {
                if (names[index] == name)
                {
                    return index;
                }
            }
            throw new InvalidOperationException("Template 실제 " + name + " 섹션 이름 누락");
        }

        /// <summary>
        /// 이름의 표준 위치를 원본 검색과 같은 ASCII 대소문자 비교로 찾는다. 이름표에 없으면 null(비표준 이름)이다.
        /// </summary>
        /// <param name="name">섹션 이름.</param>
        internal static int? CanonicalSection(string name)
        {
            IReadOnlyList<string> names = FortTemplate.SectionNames;
            // 원본 검색과 같은 ASCII 대소문자 비교로 표준 위치를 찾는다
            for (int index = 0; index < names.Count; index++)
            {
                if (RawDataManagerSelection.CompareSectionNames(names[index], name) == 0)
                {
                    return index;
                }
            }
            return null;
        }

        /// <summary>노드 핸들로 실제 노드를 찾는다. 없으면 <c>std::map::at</c>의 out_of_range에 해당하는 범위 오류다.</summary>
        /// <param name="handle">노드 핸들.</param>
        internal Stream Node(uint handle)
        {
            return Streams.TryGetValue(handle, out Stream? stream)
                ? stream
                : throw new ArgumentOutOfRangeException(nameof(handle), handle, "Template 실제 노드 핸들 범위");
        }

        /// <summary>노드 토큰(원본 data 포인터를 대신하는 값)이다. 같은 노드는 이름이 바뀌어도 같은 토큰을 쓴다.</summary>
        /// <param name="handle">노드 핸들.</param>
        internal static uint BufferToken(uint handle) => 0x10000000u + handle;

        /// <summary>같은 노드의 현재 실제 할당을 표준 파일 위치 또는 동적 버퍼에서 빌린다. 표준 위치가 아직 없으면 빈 섹션으로 늘린다.</summary>
        /// <param name="handle">노드 핸들.</param>
        internal RawSectionBuffer Buffer(uint handle)
        {
            Stream stream = Node(handle);
            if (stream.Canonical is not { } canonical)
            {
                return stream.Extra;
            }
            // C++ resize(canonical + 1)과 같이 빈 섹션으로 채운다
            while (Fort.Sections.Count <= canonical)
            {
                Fort.Sections.Add(new RawSectionBuffer());
            }
            return Fort.Sections[canonical];
        }

        /// <summary>원본 60바이트에 저장된 현재 이름을 읽는다. handle 값으로 표준 종류를 추정하지 않는다.</summary>
        /// <param name="handle">노드 핸들.</param>
        /// <exception cref="InvalidOperationException">이름 40바이트 안에 NUL이 없다.</exception>
        internal string StreamName(uint handle)
        {
            byte[] bytes = Node(handle).Bytes;
            int end = Array.IndexOf(bytes, (byte)0, NodeNameOffset, NodeNameBytes);
            if (end < 0)
            {
                throw new InvalidOperationException("Template 실제 섹션 이름 NUL 누락");
            }
            return Latin1.GetString(bytes, NodeNameOffset, end - NodeNameOffset);
        }

        /// <summary>실제 60바이트 노드를 만들고 같은 생성 수명을 부여한다. 원본 생성자는 호출자가 별도로 수행한다.</summary>
        /// <param name="canonical">현재 버퍼가 놓일 표준 위치. 비표준 이름이면 null.</param>
        /// <exception cref="OverflowException">노드 핸들이나 생성 수명이 소진됐다.</exception>
        internal uint AllocateStream(int? canonical = null)
        {
            if (_nextStreamHandle == 0 || _nextStreamHandle >= 0x10000000u)
            {
                throw new OverflowException("Template 동적 노드/생성 수명 소진");
            }
            _nextStreamGeneration = unchecked(_nextStreamGeneration + 1);
            if (_nextStreamGeneration == 0)
            {
                throw new OverflowException("Template 동적 노드/생성 수명 소진");
            }
            uint handle = _nextStreamHandle++;
            Stream stream = new() { Canonical = canonical, Generation = _nextStreamGeneration };
            if (canonical is { } index)
            {
                _seeded[index] = true;
            }
            Streams.Add(handle, stream);
            return handle;
        }

        /// <summary>저장 중인 raw 빌림의 포인터/노드를 구조 변경으로 반납하지 않기 위해 빌림이 남았는지 확인한다.</summary>
        /// <exception cref="InvalidOperationException">어느 노드든 raw 쓰기 빌림 중이다.</exception>
        internal void RequireNoAccess()
        {
            // 현재 manager의 모든 실제 노드를 검사한다
            foreach (Stream stream in Streams.Values)
            {
                if (stream.Accessing)
                {
                    throw new InvalidOperationException("Template raw 쓰기 빌림 중 섹션 구조 변경 금지");
                }
            }
        }

        /// <summary>같은 typed 커서가 반납되거나 다른 이름으로 바뀌면 이전 위치/범위를 새 노드에 쓰지 않도록 커서를 비운다.</summary>
        /// <param name="handle">반납되는 노드.</param>
        internal void RetireCursor(uint handle)
        {
            if (_stateHandle == handle)
            {
                _stateHandle = 0;
                ResetState();
            }
            if (_territoryHandle == handle)
            {
                _territoryHandle = 0;
                Territory.Reset();
            }
            if (_moneyHandle == handle)
            {
                _moneyHandle = 0;
                Money.Reset();
                _loadCursors &= ~1u;
            }
            if (_badgesHandle == handle)
            {
                _badgesHandle = 0;
                Badges.Reset();
                _loadCursors &= ~2u;
            }
            if (_technologyHandle == handle)
            {
                _technologyHandle = 0;
                Technology.Reset();
                _loadCursors &= ~4u;
            }
        }

        /// <summary>C++의 <c>state = {{}, 0, 0, false}</c>와 같이 State 커서를 비운다(인스턴스는 교체하지 않는다).</summary>
        internal void ResetState()
        {
            State.Bytes = default;
            State.Position = 0;
            State.End = 0;
            State.Present = false;
        }

        /// <summary>MemStream 소멸에서 요청한 같은 실제 raw 섹션 버퍼를 반납한다.</summary>
        /// <param name="token">노드 토큰(<see cref="BufferToken"/>).</param>
        /// <exception cref="ArgumentOutOfRangeException">토큰이 이 manager의 노드를 가리키지 않는다.</exception>
        internal void ReleaseBuffer(uint token)
        {
            if (token <= 0x10000000u || !Streams.ContainsKey(token - 0x10000000u))
            {
                throw new ArgumentOutOfRangeException(nameof(token), token, "Template 실제 버퍼 핸들 범위");
            }
            Buffer(token - 0x10000000u).Release();
        }

        /// <summary>현재 노드의 data/pos/end와 같은 raw 버퍼를 이름에 해당하는 typed 커서에 다시 연결한다.</summary>
        /// <param name="handle">노드 핸들.</param>
        internal void PublishCursor(uint handle)
        {
            RawSectionBuffer bytes = Buffer(handle);
            byte[] node = Node(handle).Bytes;
            string sectionName = StreamName(handle);
            if (RawDataManagerSelection.CompareSectionNames(sectionName, "State") == 0)
            {
                _stateHandle = handle;
                State.Bytes = bytes.Memory;
                State.Position = Read(node, 4);
                State.End = Read(node, 8);
                State.Present = Read(node, 0) != 0;
            }
            if (RawDataManagerSelection.CompareSectionNames(sectionName, "Territory") == 0)
            {
                _territoryHandle = handle;
                Territory.Set(bytes.Memory, Read(node, 4), Read(node, 8));
            }
            if (RawDataManagerSelection.CompareSectionNames(sectionName, "Money") == 0)
            {
                _moneyHandle = handle;
                Money.Set(Read(node, 4), Read(node, 8));
                _loadCursors |= 1u;
            }
            if (RawDataManagerSelection.CompareSectionNames(sectionName, "Badges") == 0)
            {
                _badgesHandle = handle;
                Badges.Set(bytes.Memory, Read(node, 4), Read(node, 8), Read(node, 0) != 0);
                _loadCursors |= 2u;
            }
            if (RawDataManagerSelection.CompareSectionNames(sectionName, "Technology") == 0)
            {
                _technologyHandle = handle;
                Technology.Set(bytes.Memory, Read(node, 4), Read(node, 8), Read(node, 0) != 0);
                _loadCursors |= 4u;
            }
        }

        /// <summary>읽기 부모가 진행한 typed 커서의 현재 위치를 같은 실제 노드에 반영한다. 생성 수명/current 선택은 바꾸지 않는다.</summary>
        /// <param name="handle">노드 핸들.</param>
        internal void SyncCursorPosition(uint handle)
        {
            // 바깥 인라인 쓰기가 진행한 raw 위치를 아직 공개하지 않은 typed 위치로 되돌리지 않는다
            if (Node(handle).Accessing)
            {
                return;
            }
            byte[] node = Node(handle).Bytes;
            if (handle == _stateHandle)
            {
                Put(node, 4, State.Position);
            }
            if (handle == _territoryHandle)
            {
                Put(node, 4, Territory.Position);
            }
            if (handle == _moneyHandle && (_loadCursors & 1u) != 0)
            {
                Put(node, 4, Money.Position);
            }
            if (handle == _badgesHandle && (_loadCursors & 2u) != 0)
            {
                Put(node, 4, Badges.Position);
            }
            if (handle == _technologyHandle && (_loadCursors & 4u) != 0)
            {
                Put(node, 4, Technology.Position);
            }
        }

        /// <summary>
        /// 공개된 요새·State의 현재 버퍼를 같은 목록에 연결한다(파일 읽기 구현 자체를 대신하지 않는다).
        /// 처음 보는 표준 위치만 목록 끝에 붙이고(current는 바꾸지 않는다), 모든 노드의 data/end/alloc 필드와 커서를 현재 외부 버퍼에 동기화한다.
        /// </summary>
        /// <exception cref="InvalidOperationException">파싱 자료의 섹션이 35칸을 넘는다.</exception>
        internal void SyncStreams()
        {
            if (Fort.Sections.Count > FortTemplate.SectionCount)
            {
                throw new InvalidOperationException("Template 실제 섹션35칸 범위");
            }
            IReadOnlyList<string> names = FortTemplate.SectionNames;
            int count = Fort.Sections.Count;
            // 파싱 자료/외부 추가 표준 위치만 최초 연결하고 원문 생성/rename로 제거된 이름을 이름표 기준으로 되살리지 않는다
            for (int index = 0; index < count; index++)
            {
                if (_seeded[index])
                {
                    continue;
                }
                uint handle = AllocateStream(index);
                byte[] node = Node(handle).Bytes;
                string sectionName = names[index];
                // 이름표 글자를 노드의 이름 위치에 그대로 옮긴다(나머지는 0)
                for (int position = 0; position < sectionName.Length; position++)
                {
                    node[NodeNameOffset + position] = (byte)sectionName[position];
                }
                uint tail = Read(Manager, 4);
                if (tail == 0)
                {
                    Put(Manager, 4, handle);
                }
                else
                {
                    uint next;
                    // 목록 끝까지 따라가 새 노드를 붙인다
                    while ((next = Read(Node(tail).Bytes, NodeNext)) != 0)
                    {
                        tail = next;
                    }
                    Put(Node(tail).Bytes, NodeNext, handle);
                }
                // 파싱/외부 자료의 최초 연결은 current를 바꾸지 않는다
                StreamOrder.Add(handle);
            }
            // 실제 목록/head/current/next는 보존하고 현재 외부 버퍼와 공개 커서만 동기화한다
            foreach ((uint handle, Stream stream) in Streams)
            {
                if (stream.Accessing)
                {
                    continue;
                }
                RawSectionBuffer buffer = Buffer(handle);
                // 크기와 용량은 int 범위라 원본의 DWORD 초과 검사(0xffffffff)에 닿지 않는다
                Put(stream.Bytes, 0, buffer.Capacity != 0 ? BufferToken(handle) : 0u);
                Put(stream.Bytes, 8, (uint)buffer.Count);
                Put(stream.Bytes, 12, (uint)buffer.Capacity);
                SyncCursorPosition(handle);
                PublishCursor(handle);
            }
        }

        /// <summary>공개 진단은 보조 순서 목록 대신 실제 head/next와 이름을 읽어 현재 섹션 연결 순서를 돌려준다.</summary>
        /// <exception cref="InvalidOperationException">사슬이 실제 노드 수보다 길다(손상/순환).</exception>
        internal List<string> SectionOrder()
        {
            List<string> result = [];
            uint current = Read(Manager, 4);
            // 실제 노드 수보다 긴 사슬은 손상/순환으로 거부한다
            while (current != 0)
            {
                if (result.Count >= Streams.Count)
                {
                    throw new InvalidOperationException("Template 실제 섹션 목록 순환");
                }
                byte[] node = Node(current).Bytes;
                int end = Array.IndexOf(node, (byte)0, NodeNameOffset, NodeNameBytes);
                if (end < 0)
                {
                    throw new InvalidOperationException("Template 섹션 이름 NUL 누락");
                }
                result.Add(Latin1.GetString(node, NodeNameOffset, end - NodeNameOffset));
                current = Read(node, NodeNext);
            }
            return result;
        }

        /// <summary>실제 원시 State와 같은 커서를 노드에 반영한 뒤 원본 선택(쓰기 허용)을 실행한다.</summary>
        internal void SelectState()
        {
            SelectLoadSection("State", 1);
        }

        /// <summary>
        /// 004bd4d0/004bcb60처럼 현재 원시 Subscriber를 읽기로 선택하고 같은 노드의 pos를 진행해 이름을 읽는다.
        /// 앞의 DWORD ID는 반환하지 않고 네 바이트만 건너뛰며, 길이(WORD)가 signed 0x8000 이상이거나 목록 256바이트 출력을 넘으면 거부한다.
        /// </summary>
        /// <returns>첫 NUL 이전까지의 이름.</returns>
        /// <exception cref="ArgumentOutOfRangeException">길이 읽기나 이름이 실제 원문 범위를 벗어난다.</exception>
        internal string ReadSubscriberName()
        {
            SyncStreams();
            DataManagerSelectionHooks hooks = new()
            {
                Stream = candidate => Node(candidate).Bytes,
                Assertion = _ => throw new InvalidOperationException("Template Subscriber 실제 읽기 섹션 선택 진단"),
            };
            uint selected = RawDataManagerSelection.SelectSection(Manager, "Subscriber", 0, hooks);
            byte[] node = Node(selected).Bytes;
            RawSectionBuffer bytes = Buffer(selected);
            // 원본 DWORD ID는 반환하지 않고 커서만 네 BYTE 진행한다
            Put(node, 4, unchecked(Read(node, 4) + 4u));
            uint position = Read(node, 4);
            uint end = Read(node, 8);
            if (position > end || position > (uint)bytes.Count || (uint)bytes.Count - position < 2)
            {
                throw new ArgumentOutOfRangeException(null, "Template Subscriber 길이 읽기의 실제 원문 범위");
            }
            uint length = bytes[(int)position] | ((uint)bytes[(int)position + 1] << 8);
            Put(node, 4, position + 2u);
            position += 2u;
            if (position > end || length >= 0x8000u || length >= 256 || length > end - position || length > (uint)bytes.Count - position)
            {
                throw new ArgumentOutOfRangeException(null, "Template Subscriber signed 길이/목록256BYTE 출력 범위");
            }
            string result = Latin1.GetString(bytes.Span.Slice((int)position, (int)length));
            Put(node, 4, position + length);
            int nul = result.IndexOf('\0');
            return nul < 0 ? result : result[..nul];
        }

        /// <summary>Territory 120바이트 읽기는 비어 있지 않은 섹션 선택(읽기)을 먼저 수행한다.</summary>
        internal void SelectTerritory()
        {
            SelectLoadSection("Territory", 0);
        }

        /// <summary>
        /// 현재 노드가 없는 읽기/빈 읽기는 원본 선택의 진단으로 남기고(명시 오류), 메타의 빈 쓰기 허용은 그대로 유지한다.
        /// 선택한 같은 노드의 typed 커서를 다시 공개한다.
        /// </summary>
        /// <param name="sectionName">섹션 이름.</param>
        /// <param name="writable">쓰기 허용 여부(비영이면 허용).</param>
        /// <returns>선택된 노드 핸들.</returns>
        internal uint SelectLoadSection(string sectionName, int writable)
        {
            SyncStreams();
            DataManagerSelectionHooks hooks = new()
            {
                // 같은 manager의 실제 목록 노드를 선택한다
                Stream = node => Node(node).Bytes,
                // 누락/읽기 불가를 가짜 빈 성공으로 바꾸지 않는다
                Assertion = _ => throw new InvalidOperationException("Template 경제/메타 실제 섹션 선택 진단"),
            };
            uint selected = RawDataManagerSelection.SelectSection(Manager, sectionName, writable, hooks);
            PublishCursor(selected);
            return selected;
        }

        /// <summary>선택 없이 typed 커서를 처음 공개할 때도 같은 원시 노드에서 현재 상태를 읽는다. current는 바꾸지 않는다.</summary>
        /// <param name="sectionName">섹션 이름.</param>
        /// <exception cref="InvalidOperationException">그 이름의 현재 노드가 없다.</exception>
        internal void LoadCursor(string sectionName)
        {
            SyncStreams();
            uint handle = 0;
            // getter는 current를 바꾸지 않고 현재 이름의 실제 노드(핸들 오름차순 첫 일치)를 빌린다
            foreach (uint candidate in Streams.Keys)
            {
                if (RawDataManagerSelection.CompareSectionNames(StreamName(candidate), sectionName) == 0)
                {
                    handle = candidate;
                    break;
                }
            }
            if (handle == 0)
            {
                throw new InvalidOperationException("Template 경제/메타 현재 노드 누락");
            }
            PublishCursor(handle);
        }

        /// <summary>선택된 노드의 이름이다. 선택이 없으면 빈 문자열이다.</summary>
        /// <exception cref="InvalidOperationException">현재 노드 이름에 NUL이 없다.</exception>
        internal string CurrentSection()
        {
            uint handle = Read(Manager, 8);
            if (handle == 0)
            {
                return "";
            }
            byte[] bytes = Node(handle).Bytes;
            int end = Array.IndexOf(bytes, (byte)0, NodeNameOffset, NodeNameBytes);
            if (end < 0)
            {
                throw new InvalidOperationException("Template 현재 섹션 이름 NUL 누락");
            }
            return Latin1.GetString(bytes, NodeNameOffset, end - NodeNameOffset);
        }

        /// <summary>manager+c에서 읽은 현재 파일 이름을 <see cref="Name"/>에 맞춰 돌려준다. 원문 선택 때 쓴 같은 이름 바이트가 원본이다.</summary>
        /// <exception cref="ArgumentOutOfRangeException">256바이트 안에 NUL이 없다.</exception>
        internal string CurrentName()
        {
            string current = TerminatedText(Manager.AsSpan(ManagerNameOffset, ManagerNameBytes), "Template 현재 manager 파일명의 NUL 범위");
            if (Name != current)
            {
                Name = current;
            }
            return Name;
        }

        /// <summary>
        /// 현재 각 섹션의 버퍼를 먼저 반납하고 같은 노드의 실제 할당을 뒤에 해제한다(원본 DataManager 소멸).
        /// </summary>
        internal void Destroy()
        {
            SyncStreams();
            DataManagerLifetimeHooks hooks = new()
            {
                // 현재 같은 60바이트 노드만 빌린다
                Stream = handle => Node(handle).Bytes,
                // 이름 변경 뒤에도 같은 data 토큰의 현재 할당만 반납한다
                ReleaseBuffer = ReleaseBuffer,
                // 버퍼 소멸이 끝난 노드만 반납한다
                ReleaseStream = handle =>
                {
                    if (!Streams.Remove(handle))
                    {
                        throw new InvalidOperationException("DataManager 같은 섹션 중복 반납");
                    }
                },
            };
            RawDataManagerLifetime.DestroyDataManager(Manager, hooks);
        }
    }
}
