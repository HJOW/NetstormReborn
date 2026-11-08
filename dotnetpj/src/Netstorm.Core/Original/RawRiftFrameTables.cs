// 실제 프레임 표를 이웃 탐색기가 읽는 코드 표로 생중계한다.
using Netstorm.Assets;

namespace Netstorm.Core.Original;

// 프레임 표 생중계 모듈을 담는 네임스페이스다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.

// 실제 프레임 표를 같은 타입 번호의 코드 표로 빌린다. 사본을 만들지 않으며 표 교체가 바로 보인다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.
public sealed class RiftFrameCodeTables : IReadOnlyList<IReadOnlyList<FrameCode>>
{
    /// <summary>같은 타입 번호의 실제 프레임 표다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>실제 프레임 표를 감싼다.</summary>
    /// <param name="frames">같은 타입 번호의 실제 프레임 표다.</param>
    public RiftFrameCodeTables(IReadOnlyList<RiftTypeFrames> frames)
    {
        ArgumentNullException.ThrowIfNull(frames);
        _frames = frames;
    }

    /// <summary>타입 수다.</summary>
    public int Count => _frames.Count;

    /// <summary>해당 타입의 코드 표를 생중계로 읽는다.</summary>
    /// <param name="index">타입 번호다.</param>
    /// <returns>같은 타입의 코드 보기다.</returns>
    public IReadOnlyList<FrameCode> this[int index] => new RowView(_frames, index);

    /// <summary>타입 순서대로 코드 표를 열거한다.</summary>
    /// <returns>코드 표 열거자다.</returns>
    public IEnumerator<IReadOnlyList<FrameCode>> GetEnumerator()
    {
        for (int i = 0; i < Count; i++)
        {
            yield return this[i];
        }
    }

    /// <summary>타입 순서대로 코드 표를 열거한다.</summary>
    /// <returns>코드 표 열거자다.</returns>
    System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();

    /// <summary>한 타입의 코드를 그때그때 변환하는 생중계 보기다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
    private sealed class RowView : IReadOnlyList<FrameCode>
    {
        /// <summary>같은 타입 번호의 실제 프레임 표다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
        private readonly IReadOnlyList<RiftTypeFrames> _frames;

        /// <summary>읽을 타입 번호다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
        private readonly int _type;

        /// <summary>타입 번호를 고정해 감싼다.</summary>
        /// <param name="frames">실제 프레임 표다.</param>
        /// <param name="type">읽을 타입 번호다.</param>
        public RowView(IReadOnlyList<RiftTypeFrames> frames, int type)
        {
            _frames = frames;
            _type = type;
        }

        /// <summary>해당 타입의 코드 수다.</summary>
        public int Count => _frames[_type].Codes.Count;

        /// <summary>해당 칸의 코드를 읽는다.</summary>
        /// <param name="index">코드 번호다.</param>
        /// <returns>변환한 코드다.</returns>
        public FrameCode this[int index]
        {
            get
            {
                RiftFrameCode code = _frames[_type].Codes[index];
                return new FrameCode((char)code.Side, (char)code.Variant, code.Number, (FrameCodeFlags)code.Flags);
            }
        }

        /// <summary>코드를 순서대로 열거한다.</summary>
        /// <returns>코드 열거자다.</returns>
        public IEnumerator<FrameCode> GetEnumerator()
        {
            for (int i = 0; i < Count; i++)
            {
                yield return this[i];
            }
        }

        /// <summary>코드를 순서대로 열거한다.</summary>
        /// <returns>코드 열거자다.</returns>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => GetEnumerator();
    }
}
