using Netstorm.Core.Original;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Client.Host;

public sealed partial class CampaignTemplateStorage
{
    private sealed partial class Image
    {
        /// <summary>
        /// 서로 다른 지역 manager의 노드와 버퍼를 같은 raw 복제 부모에 연결한다(원문 0044d230).
        /// 대상 노드 번호는 원본 manager의 노드 범위 뒤에서 시작해야 하므로 호출자가 <see cref="NextStreamHandle"/>를 미리 이어 둔다.
        /// 복제가 끝나면 대상의 새 마지막 노드를 목록 순서에 더하고 커서를 다시 공개한다.
        /// </summary>
        /// <param name="source">복제 원본 manager의 이미지.</param>
        /// <param name="sectionName">복제할 섹션 이름(표준 이름이면 표준 위치에 놓인다).</param>
        /// <exception cref="InvalidOperationException">raw 쓰기 빌림 중이거나, 노드 할당 계약이 어긋났거나, 원본 섹션이 없다(원문 섹션 누락 진단).</exception>
        /// <remarks>2026-11-02 0.134(TS4)에서 추가했다. cpppj의 <c>CloneSectionFrom</c>을 옮겼다.</remarks>
        internal void CloneSectionFrom(Image source, string sectionName)
        {
            ArgumentNullException.ThrowIfNull(source);
            RequireNoAccess();
            source.RequireNoAccess();
            SyncStreams();
            source.SyncStreams();
            uint created = 0;
            DataManagerResizeHooks hooks = new()
            {
                // 두 manager의 같은 60바이트 노드를 핸들 충돌 없이 빌린다
                Selection = new DataManagerSelectionHooks
                {
                    Stream = handle => Streams.ContainsKey(handle) ? Node(handle).Bytes : source.Node(handle).Bytes,
                },
                List = ListEffects(),
                // 원문 검색과 대상 제거 뒤 실제 새 노드를 할당한다
                AllocateStream = size =>
                {
                    if (size != 60 || created != 0 || source.Streams.ContainsKey(_nextStreamHandle))
                    {
                        throw new InvalidOperationException("로그인이미지복제의실제노드할당계약");
                    }
                    created = AllocateStream(CanonicalSection(sectionName));
                    return created;
                },
                Memory = new MemStreamHooks
                {
                    // 원본 대입은 source의 현재 data/end를 읽으며 대상 할당과 편의 사본을 섞지 않는다
                    Buffer = pointer =>
                    {
                        if (pointer <= NodeTokenBase)
                        {
                            throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "로그인이미지복제의실제버퍼범위");
                        }
                        uint handle = pointer - NodeTokenBase;
                        return Streams.ContainsKey(handle) ? Buffer(handle).Span : source.Buffer(handle).Span;
                    },
                    Allocate = size =>
                    {
                        if (created == 0)
                        {
                            throw new InvalidOperationException("로그인이미지복제의노드이전버퍼할당");
                        }
                        Buffer(created).Resize(checked((int)size));
                        return BufferToken(created);
                    },
                    // 생성 16384바이트와 대입 전 대상 버퍼만 원문 위치에서 반납한다
                    Release = ReleaseBuffer,
                    Assertion = _ => throw new InvalidOperationException("로그인이미지복제의MemStream진단"),
                },
                MissingSource = () => throw new InvalidOperationException("로그인이미지복제의원문섹션누락진단"),
            };
            uint current = RawDataManagerResize.CloneStream(Manager, source.Manager, sectionName, hooks);
            if (current != created)
            {
                throw new InvalidOperationException("로그인이미지복제의현재노드불일치");
            }
            byte[] node = Node(created).Bytes;
            RawSectionBuffer bytes = Buffer(created);
            uint end = Read(node, 8);
            if (end > (uint)bytes.Count)
            {
                throw new ArgumentOutOfRangeException(null, "로그인이미지복제의실제끝범위");
            }
            bytes.Resize((int)end);
            StreamOrder.Add(created);
            PublishCursor(created);
        }

        /// <summary>
        /// 원문 0044cf20 덤프의 파일명·이름표·현재 검색을 같은 실제 manager에서 실행한다.
        /// 출력 장치가 필터를 판정하더라도 current 검색은 생략하지 않으며, 장치가 없으면 선택된 경계 누락으로 거부한다.
        /// 근거: <c>docs/exe/cpp-datamanager-dump-20261101.md</c>.
        /// </summary>
        /// <param name="prefix">출력 접두 문자열.</param>
        /// <param name="trace">level 29 출력 장치. 실제 Client의 필터/FILE/공유 버퍼/Chat에 연결된다.</param>
        /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
        internal void Dump(string prefix, DataManagerTrace? trace)
        {
            SyncStreams();
            DataManagerDumpHooks hooks = new()
            {
                FileName = current => RawTemplateLoad.FileName(current),
                Names = _ => _sectionNames.Names(),
                Selection = new DataManagerSelectionHooks { Stream = handle => Node(handle).Bytes },
                Trace = trace,
            };
            RawDataManagerDump.Dump(Manager, prefix, hooks);
        }

        /// <summary>
        /// 파일 쓰기 없이 같은 manager의 서명/current 검색 효과로 35개 이름의 raw 이미지를 작성한다.
        /// 호출자의 장치 묶음은 바꾸지 않고(C++의 값 전달) 사본에 서명 flag와 섹션 조회를 채운다. 이름 검색은 실제 current를 갱신하며 source/target의 pos를 임의로 진행하지 않는다.
        /// </summary>
        /// <param name="hooks">타입 개수·진단.</param>
        /// <returns>0x8000 고정 할당의 이미지.</returns>
        /// <exception cref="InvalidOperationException">35개 이름 중 목록에 없는 섹션이 있다.</exception>
        /// <remarks>2026-11-02 0.134(TS4)에서 추가했다.</remarks>
        internal FortImageData SaveImage(FortImageHooks hooks)
        {
            ArgumentNullException.ThrowIfNull(hooks);
            SyncStreams();
            FortImageHooks local = new()
            {
                TypeCount = hooks.TypeCount,
                Assertion = hooks.Assertion,
                SignatureFlag = () =>
                {
                    Put(Manager, 0x10c, 0x46);
                    if (Read(Manager, 0x110) == 0xffffffffu)
                    {
                        Put(Manager, 0x110, 0);
                    }
                    return Read(Manager, 0x110) != 0 ? (byte)0xfe : (byte)0;
                },
                Section = index =>
                {
                    DataManagerSelectionHooks selection = new() { Stream = candidate => Node(candidate).Bytes };
                    uint handle = RawDataManagerSelection.FindSection(Manager, FortTemplate.SectionNames[index], selection);
                    if (handle == 0)
                    {
                        throw new InvalidOperationException("로그인이미지저장의실제섹션누락진단");
                    }
                    uint end = Read(Node(handle).Bytes, 8);
                    RawSectionBuffer bytes = Buffer(handle);
                    if (end > (uint)bytes.Count)
                    {
                        throw new ArgumentOutOfRangeException(null, "로그인이미지저장의실제끝범위");
                    }
                    return bytes.Memory[..(int)end];
                },
            };
            return new RawFortImage(OriginalEdition.Patch1078, Fort, local).Save();
        }
    }
}
