### NSA
NSA is an experimen with compression algorithms. NSA uses Burrows–Wheeler transform followed by a variant of Distance Coding, and Arithmetic Coding. It achieves mediocre compression rate, being worse than bzip2 on small files, and slightly better than bzip2 on large files. 

##### Insructions

    git clone https://github.com/nosovicki/nsa.git
    cd nsa
    make
    ./nsa 1k
    ./unsa 1k.nsa

