import Test
import QtQml

ReadCounter {
    property int finalLength: 0;

    function randomBetween(min, max) {
        const minceil = Math.ceil(min);
        const maxfloor = Math.floor(max);
        return Math.floor(Math.random() * (maxfloor - minceil) + minceil);
    }

    Component.onCompleted: {
        // 0 accesses. Will obtain data as needed.
        let bindableReference = bindable;

        let accesses = randomBetween(4, 100);
        while (accesses > 0) {
            // 1 access on first iteration. 0 after as no data is
            // changed.
            bindableReference.length;
            --accesses;
        }

        // 3 accesses: getLength, virtualPut, setLength
        //   We don't connect here since it's the same statement.
        //   In dev, QV4::Sequence is optimized to avoid this.
        bindable.push("foo");
        // 1 accesses
        finalLength = bindableReference.length;
    }
}
