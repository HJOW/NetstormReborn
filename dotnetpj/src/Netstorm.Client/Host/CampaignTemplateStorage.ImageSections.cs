using Netstorm.Core.Original;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Client.Host;

public sealed partial class CampaignTemplateStorage
{
    private sealed partial class Image
    {
        /// <summary>임시 할당의 불투명 토큰이 노드 토큰(<see cref="BufferToken"/>)과 겹치지 않게 하는 기준이다.</summary>
        private const uint TemporaryTokenBase = 0x20000000u;

        /// <summary>노드 토큰이 노드 핸들과 구별되도록 더하는 기준이다(<see cref="BufferToken"/>과 같다).</summary>
        private const uint NodeTokenBase = 0x10000000u;

        /// <summary>
        /// 토큰이 가리키는 실제 raw 버퍼 범위를 돌려준다. 노드 토큰이 아니면 범위 오류다.
        /// </summary>
        /// <param name="pointer">노드 토큰.</param>
        /// <param name="message">범위 오류 진단 문구.</param>
        private Span<byte> NodeBuffer(uint pointer, string message)
        {
            if (pointer <= NodeTokenBase || !Streams.ContainsKey(pointer - NodeTokenBase))
            {
                throw new ArgumentOutOfRangeException(nameof(pointer), pointer, message);
            }
            return Buffer(pointer - NodeTokenBase).Span;
        }

        /// <summary>
        /// 실제 임시 할당을 원본 포인터 토큰으로 빌린 뒤 완료한 같은 버퍼를 raw 섹션에 넘긴다. 연산 안에서 MemStream이 새로 할당하면
        /// (<c>MemStream::Allocate</c>) 그 할당을 임시 토큰으로 받아 두었다가, 연산이 같은 노드의 data를 그 토큰으로 바꿨을 때만 노드 토큰으로 바꿔 인계한다.
        /// 마지막에 논리 end로 크기를 줄이고(용량은 유지) 커서를 다시 공개한다.
        /// </summary>
        /// <param name="handle">대상 노드.</param>
        /// <param name="operation">노드 16바이트 MemStream 필드에 쓰는 원본 연산.</param>
        /// <exception cref="ArgumentOutOfRangeException">연산 뒤 end가 실제 버퍼 크기를 넘는다.</exception>
        internal void WithMemory(uint handle, Action<MemStreamHooks> operation)
        {
            RawSectionBuffer allocation = new();
            bool allocated = false;
            uint temporary = TemporaryTokenBase + handle;
            byte[] node = Node(handle).Bytes;
            MemStreamHooks hooks = new()
            {
                // 옛 버퍼는 free 전까지 같은 실제 소유 버퍼에서 읽는다
                Buffer = pointer => pointer == temporary && allocated
                    ? allocation.Span
                    : NodeBuffer(pointer, "Template MemStream 실제 버퍼 핸들 범위"),
                // 실제 임시 할당은 원본 본문이 0으로 채운다
                Allocate = size =>
                {
                    if (allocated)
                    {
                        throw new InvalidOperationException("Template MemStream 한 작업의 중복 할당");
                    }
                    allocation.Resize(checked((int)size));
                    allocated = true;
                    return temporary;
                },
                // 같은 원시 섹션의 옛 할당만 반납한다
                Release = pointer =>
                {
                    if (pointer != 0)
                    {
                        ReleaseBuffer(pointer);
                    }
                },
                // 잘못된 저장 버퍼를 성공으로 대신하지 않는다
                Assertion = _ => throw new InvalidOperationException("Template 실제 MemStream 진단"),
            };
            operation(hooks);
            RawSectionBuffer buffer = Buffer(handle);
            if (Read(node, 0) == temporary && allocated)
            {
                buffer.TakeFrom(allocation);
                Put(node, 0, BufferToken(handle));
            }
            uint end = Read(node, 8);
            if (end > (uint)buffer.Count)
            {
                throw new ArgumentOutOfRangeException(null, "Template MemStream 완료 뒤 실제 end 범위");
            }
            buffer.Resize((int)end);
            PublishCursor(handle);
        }

        /// <summary>
        /// 같은 stream의 물리 할당을 노출하고 원본 쓰기 뒤 논리 end만 원시 섹션에 남긴다. 입력은 호출 시점에 복사해 두므로
        /// 입력이 대상 버퍼와 겹치는 호출(같은 섹션 안의 구간을 입력으로 주는 경우)은 원본의 DWORD 단위 순방향 복사와 다를 수 있다.
        /// </summary>
        /// <param name="handle">대상 노드.</param>
        /// <param name="input">쓸 원문.</param>
        internal void WriteStream(uint handle, ReadOnlySpan<byte> input)
        {
            // 람다는 Span을 캡처할 수 없어 입력을 배열로 복사한다
            byte[] copy = input.ToArray();
            AccessSection(handle, (fields, hooks) => RawMemStream.Write(fields, copy, (uint)copy.Length, hooks));
        }

        /// <summary>원본 ce10이 새 노드를 붙이기 전에 마지막 MemStream의 end에 맞춰 할당을 줄인다. 이미 같으면 아무것도 하지 않는다.</summary>
        /// <param name="handle">마지막 노드.</param>
        internal void CompactTail(uint handle)
        {
            byte[] node = Node(handle).Bytes;
            if (Read(node, 8) == Read(node, 12))
            {
                return;
            }
            WithMemory(handle, hooks => RawMemStream.Compact(Node(handle).Bytes, hooks));
        }

        /// <summary>실제 노드/버퍼 반납과 마지막 압축을 같은 원본 목록 처리기에 공급한다.</summary>
        internal DataManagerListHooks ListEffects()
        {
            return new DataManagerListHooks
            {
                Lifetime = new DataManagerLifetimeHooks
                {
                    // 같은 현재 60바이트 노드를 빌린다
                    Stream = handle => Node(handle).Bytes,
                    // 실제 raw 섹션 할당이다
                    ReleaseBuffer = ReleaseBuffer,
                    // 버퍼 소멸을 마친 노드와 그 typed 커서만 반납한다
                    ReleaseStream = handle =>
                    {
                        RetireCursor(handle);
                        if (!Streams.Remove(handle))
                        {
                            throw new InvalidOperationException("Template 실제 노드 중복 반납");
                        }
                        StreamOrder.RemoveAll(candidate => candidate == handle);
                    },
                },
                Compact = CompactTail,
                MissingPredecessor = () => throw new InvalidOperationException("Template 실제 선행 노드 진단"),
            };
        }

        /// <summary>같은 이름의 기존 노드 unlink/버퍼 반납 뒤 이름과 독립적인 새 마지막 노드를 붙인다(원본 0044d1b0).</summary>
        /// <param name="sectionName">섹션 이름(Temp 같은 동적 이름도 허용).</param>
        /// <returns>새 마지막 노드의 핸들.</returns>
        /// <exception cref="InvalidOperationException">State 확장 복사 중이거나 raw 쓰기 빌림 중이거나 생성 부모의 계약이 어긋났다.</exception>
        internal uint CreateSection(string sectionName)
        {
            if (RetiredState.Count != 0)
            {
                throw new InvalidOperationException("State 확장 복사 중 새 섹션 생성 불가");
            }
            RequireNoAccess();
            SyncStreams();
            uint handle = 0;
            RawSectionBuffer allocation = new();
            bool allocated = false;
            DataManagerResizeHooks hooks = new()
            {
                // 검색/기존 제거/새 추가도 같은 실제 부모에서 수행한다
                Selection = new DataManagerSelectionHooks { Stream = candidate => Node(candidate).Bytes },
                List = ListEffects(),
                AllocateStream = size =>
                {
                    if (size != 60 || handle != 0)
                    {
                        throw new InvalidOperationException("Template 새 섹션 실제 노드 할당 계약 위반");
                    }
                    handle = AllocateStream(CanonicalSection(sectionName));
                    return handle;
                },
                // 새 할당은 완료 전까지 별도 소유하고 기존 tail 압축은 원래 실제 버퍼를 쓴다
                Memory = new MemStreamHooks
                {
                    Buffer = pointer => pointer == TemporaryTokenBase + handle
                        ? allocation.Span
                        : NodeBuffer(pointer, "Template 생성 부모의 실제 버퍼 범위"),
                    Allocate = size =>
                    {
                        if (allocated)
                        {
                            throw new InvalidOperationException("Template 생성 부모의 중복 할당");
                        }
                        allocation.Resize(checked((int)size));
                        allocated = true;
                        return TemporaryTokenBase + handle;
                    },
                    Assertion = _ => throw new InvalidOperationException("Template 생성 부모의 실제 MemStream 진단"),
                },
            };
            uint current = RawDataManagerResize.CreateStream(Manager, sectionName, hooks);
            byte[] node = Node(handle).Bytes;
            if (current != handle || !allocated || Read(node, 0) != TemporaryTokenBase + handle)
            {
                throw new InvalidOperationException("Template 생성 부모의 실제 현재 섹션 불일치");
            }
            Put(node, 0, BufferToken(handle));
            // 생성 직후 end는 0이므로 크기는 0으로 줄고 용량(16384)은 남는다
            allocation.Resize((int)Read(node, 8));
            Buffer(handle).TakeFrom(allocation);
            StreamOrder.Add(handle);
            PublishCursor(handle);
            return handle;
        }

        /// <summary>기존 State 생성 호출도 같은 일반 생성/원본 쓰기 부모를 사용한다(새 State를 만들고 28바이트를 쓴다).</summary>
        /// <param name="input">쓸 State 28바이트.</param>
        /// <exception cref="ArgumentException">입력이 28바이트가 아니다.</exception>
        internal void InitializeState(ReadOnlySpan<byte> input)
        {
            if (input.Length != 28)
            {
                throw new ArgumentException("Template State 생성 입력은 정확히 28바이트다");
            }
            uint handle = CreateSection("State");
            WriteStream(handle, input);
        }

        /// <summary>
        /// 현재 선택을 바꾸지 않고 명시 노드에 인라인 쓰기를 적용한 뒤 논리 끝만 원시 섹션에 공개한다. 연산 중에는 이 노드가 빌림 상태이므로
        /// 같은 노드의 중첩 쓰기와 섹션 구조 변경이 거부된다. 연산이 예외를 던져도 그 전까지의 pos/end/부분 쓰기는 보존하고 물리 할당을 논리 자료로 공개하지 않는다.
        /// </summary>
        /// <param name="handle">대상 노드.</param>
        /// <param name="operation">연산.</param>
        /// <exception cref="ArgumentException">연산이 없다.</exception>
        /// <exception cref="InvalidOperationException">같은 노드가 이미 빌림 중이다.</exception>
        internal void AccessSection(uint handle, TemplateSectionAccess operation)
        {
            if (operation is null)
            {
                throw new ArgumentException("Template 인라인 쓰기 장치 누락");
            }
            Stream stream = Node(handle);
            if (stream.Accessing)
            {
                throw new InvalidOperationException("Template 같은 stream의 중첩 쓰기 빌림 금지");
            }
            SyncCursorPosition(handle);
            RawSectionBuffer buffer = Buffer(handle);
            byte[] node = stream.Bytes;
            uint allocation = Read(node, 12);
            // 쓰기 범위가 실제 물리 할당 전체를 덮도록 크기를 할당까지 늘린다
            if ((uint)buffer.Count < allocation)
            {
                buffer.Resize(checked((int)allocation));
            }
            stream.Accessing = true;
            try
            {
                WithMemory(handle, hooks => operation(node.AsSpan(0, 16), hooks));
            }
            catch
            {
                // 예외 전의 pos/end/부분 쓰기를 보존하며 물리 할당을 논리 자료로 잘못 공개하지 않는다
                stream.Accessing = false;
                uint end = Read(node, 8);
                if (end <= (uint)buffer.Count)
                {
                    buffer.Resize((int)end);
                }
                PublishCursor(handle);
                throw;
            }
            stream.Accessing = false;
        }

        /// <summary>
        /// 실제 이름 교체가 끝난 같은 노드의 할당을 새 표준 위치/동적 저장소로 옮긴다. 바이트를 복사하지 않고 같은 할당(배열)을 넘긴다.
        /// 표준 위치가 그대로면 커서만 다시 공개한다.
        /// </summary>
        /// <param name="handle">이름이 바뀐 노드.</param>
        internal void BindRenamedBuffer(uint handle)
        {
            Stream stream = Node(handle);
            int? destination = CanonicalSection(StreamName(handle));
            if (destination == stream.Canonical)
            {
                PublishCursor(handle);
                return;
            }
            RawSectionBuffer allocation = new();
            allocation.TakeFrom(Buffer(handle));
            RetireCursor(handle);
            stream.Canonical = destination;
            if (destination is { } index)
            {
                _seeded[index] = true;
            }
            Buffer(handle).TakeFrom(allocation);
            PublishCursor(handle);
        }

        /// <summary>이름 검색은 원문처럼 현재 선택(current)을 갱신하되 현재 위치와 목록을 바꾸지 않는다. 없으면 0이다.</summary>
        /// <param name="sectionName">섹션 이름.</param>
        internal uint FindSection(string sectionName)
        {
            SyncStreams();
            DataManagerSelectionHooks hooks = new() { Stream = candidate => Node(candidate).Bytes };
            return RawDataManagerSelection.FindSection(Manager, sectionName, hooks);
        }

        /// <summary>원본 Rename이 대상 제거/현재 source 재검색/이름 쓰기를 끝낸 뒤 버퍼의 호스트 위치만 매핑한다.</summary>
        /// <param name="destination">새 이름.</param>
        /// <param name="source">현재 이름.</param>
        /// <returns>이름이 바뀐 노드(원래 source 노드).</returns>
        internal uint RenameSection(string destination, string source)
        {
            RequireNoAccess();
            SyncStreams();
            DataManagerResizeHooks hooks = new()
            {
                Selection = new DataManagerSelectionHooks { Stream = candidate => Node(candidate).Bytes },
                List = ListEffects(),
            };
            uint current = RawDataManagerResize.RenameStream(Manager, destination, source, hooks);
            BindRenamedBuffer(current);
            return current;
        }

        /// <summary>
        /// 실제 임시 노드가 살아 있는 동안 원본 부모(0044d290)를 실행하고 완료한 같은 할당을 State로 넘긴다.
        /// 현재 State의 end가 최소보다 작을 때만 임시 노드를 만들고, 충분하면 아무것도 바꾸지 않고 커서만 다시 공개한다.
        /// </summary>
        /// <param name="minimum">필요한 최소 end(signed).</param>
        /// <exception cref="InvalidOperationException">State 확장 복사 중이거나 raw 쓰기 빌림 중이거나 임시 소유권 계약이 어긋났다.</exception>
        internal void EnsureStateSize(int minimum)
        {
            if (RetiredState.Count != 0)
            {
                throw new InvalidOperationException("분리 State 복사 중 원본 최소 확장 불가");
            }
            RequireNoAccess();
            SyncStreams();
            uint temporary = 0;
            RawSectionBuffer buffer = new();
            bool allocated = false;
            DataManagerResizeHooks hooks = new()
            {
                // 원문과 임시 노드 모두 같은 실제 저장소에 둔다
                Selection = new DataManagerSelectionHooks { Stream = candidate => Node(candidate).Bytes },
                List = ListEffects(),
                // 복사는 원본 부모가 요청하는 현재 data/end를 사용한다
                Memory = new MemStreamHooks
                {
                    Buffer = pointer => pointer == TemporaryTokenBase + temporary
                        ? buffer.Span
                        : NodeBuffer(pointer, "Template 확장의 실제 원문 버퍼 핸들 범위"),
                    Allocate = size =>
                    {
                        if (allocated)
                        {
                            throw new InvalidOperationException("Template 확장 임시 버퍼 중복 할당");
                        }
                        buffer.Resize(checked((int)size));
                        allocated = true;
                        return TemporaryTokenBase + temporary;
                    },
                    Release = pointer =>
                    {
                        if (pointer != 0)
                        {
                            ReleaseBuffer(pointer);
                        }
                    },
                    Assertion = _ => throw new InvalidOperationException("Template 원본 확장의 MemStream 진단"),
                },
                AllocateStream = size =>
                {
                    if (size != 60 || temporary != 0)
                    {
                        throw new InvalidOperationException("Template 실제 임시 노드 할당 계약 위반");
                    }
                    temporary = AllocateStream();
                    return temporary;
                },
                MissingSource = () => throw new InvalidOperationException("Template 원본 State 확장의 원문 섹션 누락"),
            };
            uint current = RawDataManagerResize.EnsureStreamSize(Manager, "State", minimum, hooks);
            if (temporary == 0)
            {
                PublishCursor(current);
                return;
            }
            byte[] node = Node(current).Bytes;
            if (current != temporary || !allocated || Read(node, 0) != TemporaryTokenBase + temporary)
            {
                throw new InvalidOperationException("Template 원본 확장의 임시 소유권 계약 위반");
            }
            uint end = Read(node, 8);
            if (end > (uint)buffer.Count)
            {
                throw new ArgumentOutOfRangeException(null, "Template 원본 확장 결과의 실제 end 범위");
            }
            // 원문 Rename의 현재/head/next와 임시 node handle을 그대로 유지하고 같은 할당만 인계한다
            Put(node, 0, BufferToken(current));
            buffer.Resize((int)end);
            Buffer(current).TakeFrom(buffer);
            StreamOrder.Add(current);
            BindRenamedBuffer(current);
        }
    }
}
