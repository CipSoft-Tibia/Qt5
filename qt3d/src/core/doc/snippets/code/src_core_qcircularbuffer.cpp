// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [0]
QCircularBuffer<int> integerBuffer;
QCircularBuffer<QString> stringBuffer;
//! [0]


//! [1]
QCircularBuffer<QString> circ(200);
//! [1]


//! [2]
QCircularBuffer<QString> circ(200, "Pass");
//! [2]


//! [3]
QCircularBuffer<QString> circ(200, 50, "Qt");
//! [3]


//! [4]
if (circ[0] == "Izzie")
  circ[0] = "Elizabeth";
//! [4]


//! [5]
for (int i = 0; i < circ.size(); ++i) {
  if (circ.at(i) == "Jack")
    cout << "Found Jack at position " << i << endl;
 }
//! [5]


//! [6]
// Create a circular buffer with capacity for 3 integers
QCircularBuffer<int> circ(3);

// Insert some items into the buffer
circ.append( 1 );
circ.append( 2 );
circ.append( 3 );

for (int i = 0; i < circ.size(); ++i) {
  cout << circ.at(i) << endl;
 } // Prints out 1 2 3

// The circular buffer is now full. Appending subsequent items
// will overwrite the oldest items:
circ.append(4); // Overwrite 1 with 4
circ.append(5); // Overwrite 2 with 5

// The buffer now contains 3, 4 and 5

for (int i = 0; i < circ.size(); ++i) {
  cout << circ.at(i) << endl;
 } // Prints out 3 4 5
//! [6]


//! [7]
int i = circ.indexOf("Tom");
if (i != -1)
  cout << "First occurrence of Tom is at position " << i << endl;
//! [7]


//! [8]
QCircularBuffer<QString> circ(3);
circ.append("one");
circ.append("two");
circ.append("three");
// circ: ["one", "two", "three"]

circ.append("four");
// circ: ["two", "three", "four"]

circ.append("five");
// circ: ["three", "four", "five"]

circ.append("six");
// circ: ["four", "five", "six"]
//! [8]


//! [9]
QCircularBuffer<int> circ(10, 0);
QCircularBuffer<int>::array_range data = circ.data();
for (int i = 0; i < data.second; ++i)
    data.first[i] = 2 * i;
//! [9]


//! [10]
QCircularBuffer<QString> circ(3);
circ.prepend("one");
circ.prepend("two");
circ.prepend("three");
// circ: ["three", "two", "one"]

circ.prepend("four");
// circ: ["four", "three", "two"]
//! [10]


//! [11]
QCircularBuffer<QString> circ(5);
circ << "alpha" << "beta" << "delta";
circ.insert(2, "gamma");
// circ: ["alpha", "beta", "gamma", "delta"]
//! [11]

//! [12]
QCircularBuffer<int> circ(6);
circ << 1 << 2 << 3 << 4; // circ: [1, 2, 3, 4]
circ.insert(2, 5, 0);
// circ: [0, 0, 0, 0, 3, 4]
//! [12]


//! [13]
QList<double> list;
list << 2.718 << 1.442 << 0.4342;
list.insert(1, 3, 9.9);
// list: [2.718, 9.9, 9.9, 9.9, 1.442, 0.4342]
//! [13]


//! [14]
QCircularBuffer<QString> circ(3, 3, "No");
circ.fill("Yes");
// circ: ["Yes", "Yes", "Yes"]

circ.fill("oh", 5);
// circ: ["oh", "oh", "oh", "oh", "oh"]
//! [14]


//! [15]
QCircularBuffer<QString> circ(5);
circ << "A" << "B" << "C" << "B" << "A";
circ.indexOf("B");            // returns 1
circ.indexOf("B", 1);         // returns 1
circ.indexOf("B", 2);         // returns 3
circ.indexOf("X");            // returns -1
//! [15]


//! [16]
QCircularBuffer<QString> circ;
circ << "A" << "B" << "C" << "B" << "A";
circ.lastIndexOf("B");        // returns 3
circ.lastIndexOf("B", 3);     // returns 3
circ.lastIndexOf("B", 2);     // returns 1
circ.lastIndexOf("X");        // returns -1
//! [16]


//! [17]
QCircularBuffer<QString> circ;
circ << "red" << "green" << "blue" << "black";

QList<QString> list = circ.toList();
// list: ["red", "green", "blue", "black"]
//! [17]


//! [18]
QStringList list;
list << "Sven" << "Kim" << "Ola";

QCircularBuffer<QString> circ = QCircularBuffer<QString>::fromList(list);
// circ: ["Sven", "Kim", "Ola"]
//! [18]


//! [19]
QCircularBuffer<int> circ(6);
circ << 1 << 2 << 3 << 4 << 5;
// circ: [1, 2, 3, 4, 5]
//! [19]


//! [20]
circ.append(6);
// circ: [1, 2, 3, 4, 5, 6]
//! [20]


//! [21]
circ.append(7);
// circ: [2, 3, 4, 5, 6, 7]
//! [21]
