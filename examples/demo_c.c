// CodeForge: C-Style Demonstration Program
int x = 10;
float y = 20.5;
bool flag = true;

int result = 0;

if (x < 15 && flag) {
    result = x * 2 + 5;
    print(result);
} else {
    result = 0;
    print(result);
}

while (x > 0) {
    x = x - 2;
    print(x);
}