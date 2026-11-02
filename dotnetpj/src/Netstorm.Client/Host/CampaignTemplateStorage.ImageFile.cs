using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Client.Host;

public sealed partial class CampaignTemplateStorage
{
    /// <summary>선택한 파일 장치를 호출한 자리에서만 요구하며 미선택 분기를 선행 거부하지 않는다.</summary>
    /// <typeparam name="T">장치 대리자 형태.</typeparam>
    /// <param name="device">요구할 장치.</param>
    /// <exception cref="InvalidOperationException">장치가 연결되지 않았다.</exception>
    private static T NeedFile<T>(T? device) where T : class
    {
        return device ?? throw new InvalidOperationException("Template 실제 파일 Load 장치 미연결");
    }

    private sealed partial class Image
    {
        /// <summary>파일 읽기 부모가 입력 파일 할당을 가리키는 불투명 토큰이다. 노드 토큰·임시 토큰과 겹치지 않는다.</summary>
        private const uint FileInputToken = 0x30000u;

        /// <summary>저장 부모가 이 동기 호출 동안만 쓰는 이미지 할당의 불투명 토큰이다.</summary>
        private const uint SavedImageToken = 0x50000u;

        /// <summary>섹션 노드 안 next DWORD의 위치다(<see cref="SectionOrder"/>의 목록 순회와 같은 값).</summary>
        private const int FileNodeNext = 56;

        /// <summary>
        /// 실제 head/next의 같은 버퍼만 표준 위치로 옮기고 current/pos/생성 수명은 재설정하지 않는다. 원문이 이미 붙인 노드만 같은 순서로 공개하며
        /// 미완료 임시 노드를 붙이지 않는다.
        /// </summary>
        /// <exception cref="InvalidOperationException">사슬이 실제 노드 수보다 길다(손상/순환).</exception>
        internal void BindFileStreams()
        {
            StreamOrder.Clear();
            uint current = Read(Manager, 4);
            while (current != 0)
            {
                if (StreamOrder.Count >= Streams.Count)
                {
                    throw new InvalidOperationException("Template 파일 읽기 뒤 실제 목록 순환");
                }
                byte[] node = Node(current).Bytes;
                StreamOrder.Add(current);
                BindRenamedBuffer(current);
                RawSectionBuffer buffer = Buffer(current);
                uint end = Read(node, 8);
                if (end > (uint)buffer.Count)
                {
                    throw new ArgumentOutOfRangeException(null, "Template 파일 읽기 뒤 실제 end 범위");
                }
                buffer.Resize((int)end);
                PublishCursor(current);
                current = Read(node, FileNodeNext);
            }
        }

        /// <summary>
        /// 같은 raw reader의 bool을 유지한다. 빈 manager(노드도 head도 없는)이면 35칸 빈 섹션으로 시작하고, 그렇지 않으면 현재 목록과 동기화한다.
        /// 새 임시 버퍼는 기존 같은 이름을 제거하기 전까지 별개로 소유한다. 호스트 예외에서도 원문이 이미 붙인 노드·부분 pos를 숨기거나 되돌리지 않는다.
        /// </summary>
        /// <param name="fileEffects">파일 읽기·이름 비교·서명 처리 장치(읽기·타입 수·진단·거부 요청이 필요하다).</param>
        /// <returns>읽기와 서명이 성공하면 true, 파일 없음·서명 거부는 false.</returns>
        /// <exception cref="InvalidOperationException">raw 쓰기 빌림 중이거나 선택된 장치가 연결되지 않았거나 원본 진단에 닿았다.</exception>
        internal bool ReadFile(TemplateFileLoadHooks fileEffects)
        {
            ArgumentNullException.ThrowIfNull(fileEffects);
            RequireNoAccess();
            if (Streams.Count == 0 && Read(Manager, 4) == 0)
            {
                // 빈 manager는 35칸 빈 요새로 시작한다(원문 읽기가 모든 표준 이름을 채운다)
                Fort.Sections.Clear();
                for (int index = 0; index < FortTemplate.SectionCount; index++)
                {
                    Fort.Sections.Add(new RawSectionBuffer());
                }
                Array.Fill(_seeded, true);
                ResetState();
                Territory.Reset();
            }
            else
            {
                SyncStreams();
            }
            // 파일 할당은 지역 소유지만 이름표는 같은 Client에서 빌린다
            byte[] source = [];
            uint allocating = 0;
            uint created = 0;
            DataManagerImageHooks hooks = new()
            {
                Names = () => _sectionNames.Names(),
                Assertion = _ => throw new InvalidOperationException("Template 실제 파일 이미지의 원본 진단"),
                Streams = new DataManagerResizeHooks
                {
                    Selection = new DataManagerSelectionHooks { Stream = candidate => Node(candidate).Bytes },
                    List = ListEffects(),
                    // 기존 canonical 버퍼는 원문 제거 전까지 덮어쓰지 않는다
                    AllocateStream = size =>
                    {
                        if (size != 60 || created >= FortTemplate.SectionCount)
                        {
                            throw new InvalidOperationException("Template 파일 이미지의 새 노드 계약 위반");
                        }
                        ++created;
                        allocating = AllocateStream();
                        return allocating;
                    },
                    // 입력과 새 임시 할당 및 기존 노드 모두 현재 실제 버퍼를 빌린다
                    Memory = new MemStreamHooks
                    {
                        Buffer = pointer => pointer == FileInputToken
                            ? source
                            : NodeBuffer(pointer, "Template 파일 이미지의 실제 버퍼 토큰 범위"),
                        Allocate = size =>
                        {
                            RawSectionBuffer buffer = Buffer(allocating);
                            if (buffer.Count != 0)
                            {
                                throw new InvalidOperationException("Template 파일 이미지의 새 버퍼 중복 할당");
                            }
                            buffer.Resize(checked((int)size));
                            return BufferToken(allocating);
                        },
                        Assertion = _ => throw new InvalidOperationException("Template 파일 이미지의 실제 MemStream 진단"),
                    },
                },
            };
            DataManagerFileHooks file = new()
            {
                Image = hooks,
                FileName = () => RawTemplateLoad.FileName(Manager),
                // manager/header가 공개되고 readonly/+16을 쓴 뒤에만 실제 파일 장치를 호출한다
                ReadFile = (fileName, mode, length, flags) =>
                {
                    if (mode != 1 || flags != 0 || length.Length != 4)
                    {
                        throw new InvalidOperationException("Template 파일 읽기의 실제 요청 인수 불일치");
                    }
                    source = NeedFile(fileEffects.Read)(TemplateFileNames.FromManager(fileName)) ?? throw new InvalidOperationException("Template 실제 파일 Load 장치 미연결");
                    Put(length, 0, (uint)source.Length);
                    return source.Length == 0 ? 0u : FileInputToken;
                },
                // 기존 원문 reader가 성공/거부/예외에서 한 번만 반납한다
                ReleaseFile = pointer =>
                {
                    if (pointer != FileInputToken)
                    {
                        throw new InvalidOperationException("Template 파일 읽기 부모의 실제 할당 반납 불일치");
                    }
                    source = [];
                },
            };
            file.Signature = cursor =>
            {
                TemplateFileSignatureHooks signature = new()
                {
                    Buffer = hooks.Streams.Memory.Buffer,
                    TypeCount = () => NeedFile(fileEffects.TypeCount)(),
                    FileName = file.FileName,
                    TooManyTypes = () => NeedFile(fileEffects.TooManyTypes)(),
                    RejectFile = fileName => NeedFile(fileEffects.RejectFile)(TemplateFileNames.FromManager(fileName)),
                };
                return RawDataManagerImage.ReadTemplateFileSignature(Manager, cursor, signature);
            };
            try
            {
                bool success = RawDataManagerImage.ReadFile(Manager, file);
                BindFileStreams();
                if (success && created != FortTemplate.SectionCount)
                {
                    throw new InvalidOperationException("Template 실제 파일의 전체 이름 표 불일치");
                }
                return success;
            }
            catch
            {
                // host 예외 전에 원문이 이미 붙인 노드/부분 pos를 숨기거나 rollback하지 않는다
                BindFileStreams();
                throw;
            }
        }

        /// <summary>
        /// 기존 Campaign/목록 교체 API는 이전과 같이 빈 manager에 읽고 정상 false를 예외로 바꾼다.
        /// </summary>
        /// <param name="input">파일 전체 바이트.</param>
        /// <param name="typeCount">서명 검사가 도달한 순간의 현재 signed TYPE 개수.</param>
        /// <exception cref="ArgumentException">빈 manager가 아니다.</exception>
        /// <exception cref="InvalidOperationException">파일 열기 또는 서명이 거부됐다.</exception>
        /// <exception cref="InvalidDataException">잘못된 서명의 원본 파일 제거 요청이다(보호 원문은 삭제하지 않는다).</exception>
        internal void ReadFile(ReadOnlySpan<byte> input, int typeCount)
        {
            if (Streams.Count != 0 || Read(Manager, 4) != 0)
            {
                throw new ArgumentException("Template 실제 파일 이미지/빈 manager 계약 위반");
            }
            byte[] copy = input.ToArray();
            TemplateFileLoadHooks file = new()
            {
                Read = _ => copy,
                TypeCount = () => typeCount,
                TooManyTypes = () => throw new InvalidOperationException("Template 파일의 numTypes<256 진단"),
                RejectFile = _ => throw new InvalidDataException("Template 잘못된 서명의 원본 파일 제거 요청: 보호 원문은 삭제하지 않습니다"),
            };
            if (!ReadFile(file))
            {
                throw new InvalidOperationException("Template 실제 파일 열기/서명 거부");
            }
        }

        /// <summary>
        /// 현재 manager의 서명/이름 검색 효과를 같은 파일 저장 부모에 공급한다. 이미지 생성은 같은 manager의 현재 섹션을 35개 이름 순서로 읽어 직렬화한다.
        /// 서명 flag는 manager+10c를 'F'(0x46)로, FE 판정(+110)이 미설정(0xffffffff)이면 0으로 쓴 뒤 그 현재 값으로 정한다.
        /// </summary>
        /// <param name="stateInputs">저장 억제·로컬 소유자·숨김 전역의 현재 값.</param>
        /// <param name="imageHooks">타입 개수·진단. 서명 flag와 섹션 조회 장치는 이 호출이 덮어쓴 사본에 채운다.</param>
        /// <param name="file">권한 변경·쓰기 장치.</param>
        /// <exception cref="InvalidOperationException">장치가 연결되지 않았거나 실제 파일 저장의 누락 섹션 진단에 닿았다.</exception>
        internal void SaveFile(DataManagerFileSaveState stateInputs, FortImageHooks imageHooks, TemplateFileWriteHooks file)
        {
            ArgumentNullException.ThrowIfNull(stateInputs);
            ArgumentNullException.ThrowIfNull(imageHooks);
            ArgumentNullException.ThrowIfNull(file);
            SyncStreams();
            FortImageData? image = null;
            // C++은 이미지 장치를 값으로 받으므로 호출자의 객체는 바꾸지 않고 사본에 이 호출의 장치를 채운다
            FortImageHooks local = new()
            {
                TypeCount = imageHooks.TypeCount,
                Assertion = imageHooks.Assertion,
                SignatureFlag = () =>
                {
                    Put(Manager, 0x10c, 0x46);
                    if (Read(Manager, 0x110) == 0xffffffffu)
                    {
                        Put(Manager, 0x110, 0);
                    }
                    return Read(Manager, 0x110) != 0 ? (byte)0xfe : (byte)0;
                },
                // 검색은 같은 목록/current를 바꾸며 Source stream의 pos는 초기화하지 않는다
                Section = SectionByIndex,
            };
            DataManagerFileSaveHooks hooks = new()
            {
                Image = (source, stream) =>
                {
                    if (!Unsafe.AreSame(ref MemoryMarshal.GetReference(source), ref MemoryMarshal.GetArrayDataReference(Manager)))
                    {
                        throw new InvalidOperationException("Template 저장의 실제 manager 불일치");
                    }
                    image = new RawFortImage(OriginalEdition.Patch1078, Fort, local).Save();
                    Span<byte> fields = stream.Span;
                    Put(fields, 0, SavedImageToken);
                    Put(fields, 4, 0);
                    Put(fields, 8, (uint)image.End);
                    Put(fields, 12, (uint)image.Storage.Length);
                },
                FileName = source => RawTemplateLoad.FileName(source),
                Permissions = (name, mode) => (file.Permissions ?? throw new InvalidOperationException("Template 실제 파일 권한 장치 미연결"))(TemplateFileNames.FromManager(name), mode),
                Write = (name, pointer, length) =>
                {
                    if (pointer != SavedImageToken || image is null || length > (uint)image.Storage.Length)
                    {
                        throw new ArgumentOutOfRangeException(null, "Template 파일 저장의 실제 이미지 포인터/길이 범위");
                    }
                    (file.Write ?? throw new InvalidOperationException("Template 실제 파일 쓰기 장치 미연결"))(TemplateFileNames.FromManager(name), image.Storage.AsSpan(0, (int)length));
                },
                Release = pointer =>
                {
                    if (pointer != SavedImageToken)
                    {
                        throw new InvalidOperationException("Template 저장 이미지 할당 반납 불일치");
                    }
                    image = null;
                },
            };
            RawDataManagerFileSave.Save(Manager, stateInputs, hooks);
        }

        /// <summary>
        /// 이미지 생성이 요구하는 표준 위치 <paramref name="index"/>의 현재 섹션 내용을 실제 목록에서 이름으로 찾아 돌려준다(원본 검색은 current를 갱신한다).
        /// 파일 저장은 35개 이름을 모두 조회하므로 목록에 없으면 명시 오류다.
        /// </summary>
        /// <param name="index">0~34 표준 이름 위치.</param>
        /// <exception cref="InvalidOperationException">그 이름의 섹션이 목록에 없다.</exception>
        private ReadOnlyMemory<byte> SectionByIndex(int index)
        {
            DataManagerSelectionHooks selection = new() { Stream = candidate => Node(candidate).Bytes };
            uint handle = RawDataManagerSelection.FindSection(Manager, FortTemplate.SectionNames[index], selection);
            if (handle == 0)
            {
                throw new InvalidOperationException("Template 실제 파일 저장의 누락 섹션 진단");
            }
            RawSectionBuffer bytes = Buffer(handle);
            uint end = Read(Node(handle).Bytes, 8);
            if (end > (uint)bytes.Count)
            {
                throw new ArgumentOutOfRangeException(null, "Template 저장 섹션의 실제 end 범위");
            }
            return bytes.Memory[..(int)end];
        }
    }
}
