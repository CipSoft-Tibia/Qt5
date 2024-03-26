// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//Will be initialized when WorkerScript{} is instantiated
var cache = new Array(64);
for (var i = 0; i < 64; i++)
    cache[i] = new Array(64);

function triangle(row, column) {
    if (cache[row][column])
        return cache[row][column]
    if (column < 0 || column > row)
        return -1;
    if (column == 0 || column == row)
        return 1;
    return triangle(row-1, column-1) + triangle(row-1, column);
}
//! [0]
WorkerScript.onMessage = function(message) {
    //Calculate result (may take a while, using a naive algorithm)
    var calculatedResult = triangle(message.row, message.column);
    //Send result back to main thread
    WorkerScript.sendMessage( { row: message.row,
                                column: message.column,
                                result: calculatedResult} );
}
//! [0]
