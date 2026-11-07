// CodeForge: Java-Style Demonstration Program
int i = 0;
int max = 10;
int sum = 0;
bool done = false;

while (i < max && !done) {
    sum = sum + i;
    i = i + 1;
    
    if (sum > 20) {
        done = true;
    }
}

print(sum);
print(done);