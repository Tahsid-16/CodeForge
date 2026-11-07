// CodeForge: C++ Demo
int score = 85;
bool passed = false;

if (score >= 50) {
    passed = true;
    
    // Nested scope block
    int bonus = 10;
    score = score + bonus;
    cout << score;
}

cout << passed;