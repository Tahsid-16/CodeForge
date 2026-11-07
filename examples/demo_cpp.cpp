// CodeForge: C++-Style Demonstration Program
int count = 5;
float rate = 1.25;
bool is_valid = false;

float total = count * rate;

if (total >= 5.0) {
    is_valid = true;
    
    // Nested scope testing
    int inner_multiplier = 3;
    int scoped_result = count * inner_multiplier;
    print(scoped_result);
}

print(is_valid);