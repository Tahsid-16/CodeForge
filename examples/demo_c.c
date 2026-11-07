// CodeForge: C Demo
int base = 10;
float rate = 2.5;
int iterations = 4;
int counter = 0;

while (counter < iterations) {
    base = base + 5;
    counter = counter + 1;
}

if (base > 20) {
    printf(base);
} else {
    printf(counter);
}