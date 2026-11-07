// CodeForge: Java Demo
int sum = 0;
int limit = 5;
boolean is_complete = false;

for (int step = 1; step <= limit; step = step + 1) {
    sum = sum + step;
    if (sum >= 10) {
        is_complete = true;
    }
}

System.out.println(sum);
System.out.println(is_complete);