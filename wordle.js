
const fs = require('fs');
const readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
  });
  
  
const Condition = function (inWord, notInWord) {
    this.inWord = inWord || {};
    this.notInWord = notInWord || new Set();
};

const WordleGame = function (words, validGuesses, baseEntropy) {
    this.words = words;
    this.validGuesses = validGuesses;
    if (baseEntropy === -1) {
        this.baseEntropy = this.conditionalEntropy(words);
    } else {
        this.baseEntropy = baseEntropy;
    }
};

WordleGame.prototype.wordIsCompatible = function (word, condition) {
    let isCompatible = true;
    for (let letter in condition.inWord) {
        // Letter should be in word
        if (word.indexOf(letter) === -1) {
            return false;
        }
        for (let position in condition.inWord[letter]) {
            if (condition.inWord[letter][position] !== (word[position] === letter)) {
                return false;
            }
        }
    }
    for (let letter of condition.notInWord) {
        // Letter should not be in word
        if (word.indexOf(letter) !== -1) {
            return false;
        }
    }
    return true;
};

WordleGame.prototype.getCompatibleWords = function (condition) {
    const compatibleWords = [];
    for (let word of this.words) {
        if (this.wordIsCompatible(word, condition)) {
            compatibleWords.push(word);
        }
    }
    return compatibleWords;
};

WordleGame.prototype.conditionalEntropy = function (words, condition, verbose) {
    const compatibleWords = condition ? this.getCompatibleWords(condition) : this.words;
    // Entropy is the sum of the probabilities of each word multiplied by the log of the probability. Assume each word remaining is equally likely.
    let entropy = 0;
    for (let word of compatibleWords) {
        const probability = 1.0 / compatibleWords.length;
        entropy += probability * Math.log2(probability);
    }
    return -entropy;
};

WordleGame.prototype.evaluateGuessToCondition = function (groundTruth, guess) {
    const condition = new Condition();
    for (let i = 0; i < 5; i++) {
        if (groundTruth.indexOf(guess[i]) !== -1) {
            condition.inWord[guess[i]] = {};
            condition.inWord[guess[i]][i] = guess[i] === groundTruth[i];
        } else {
            condition.notInWord.add(guess[i]);
        }
    }
    return condition;
};

WordleGame.prototype.combineConditions = function (condition1, condition2) {
    const condition = new Condition();
    for (let letter in condition1.inWord) {
        for (let position in condition1.inWord[letter]) {
            condition.inWord[letter] = {};
            condition.inWord[letter][position] = condition1.inWord[letter][position];
        }
    }
    for (let letter of condition1.notInWord) {
        condition.notInWord.add(letter);
    }
    for (let letter in condition2.inWord) {
        for (let position in condition2.inWord[letter]) {
            condition.inWord[letter] = {};
            condition.inWord[letter][position] = condition2.inWord[letter][position];
        }
    }
    for (let letter of condition2.notInWord) {
        condition.notInWord.add(letter);
    }
    // Remove from condition.notInWord any letters that are in condition.inWord
    for (let letter in condition.inWord) {
        condition.notInWord.delete(letter);
    }
    return condition;
};

WordleGame.prototype.expectedInformationGain = function (guess, condition) {
    const informationGains = [];
    for (let i = 0; i < this.words.length; i++) {
        const groundTruth = this.words[i];
        const newCondition = this.evaluateGuessToCondition(groundTruth, guess);
        const combinedCondition = condition ? this.combineConditions(condition, newCondition) : newCondition;
        informationGains.push(this.baseEntropy - this.conditionalEntropy(this.words, combinedCondition));
    }
    return informationGains.reduce((a, b) => a + b) / informationGains.length;
};

WordleGame.prototype.convertPrettyConditionToCondition = function (guess, prettyCondition) {
    // prettyCondition is a string of the form "nnypn" where:
    // g means the corresponding letter in guess is in the word at the corresponding position
    // y means the corresponding letter in guess is in the word but not at the corresponding position
    // b means the corresponding letter in guess is not in the word
    const condition = new Condition();
    for (let i = 0; i < 5; i++) {
        if (prettyCondition[i] === 'g') {
            condition.inWord[guess[i]] = {};
            condition.inWord[guess[i]][i] = true;
        } else if (prettyCondition[i] === 'y') {
            condition.inWord[guess[i]] = {};
            condition.inWord[guess[i]][i] = false;
        } else if (prettyCondition[i] === 'b') {
            condition.notInWord.add(guess[i]);
        }
    }
    return condition;
};

WordleGame.prototype.applyCondition = function (condition) {
    // Remove words that don't match the condition
    const compatibleWords = this.getCompatibleWords(condition);
    this.words = compatibleWords;
    this.baseEntropy = this.conditionalEntropy(this.words);
};

WordleGame.prototype.findOptimalGuesses = function (condition) {
    // Find the optimal guess for information gain and for compatibility
    let bestGuessForInformationGain = '';
    let bestGuessForWin = '';
    let bestInformationGain = 0;
    let bestInformationGainForWin = 0;
    const informationGains = {};
    for (let guess of this.validGuesses) {
        const thisInformationGain = this.expectedInformationGain(guess, condition);
        if (thisInformationGain === thisInformationGain && thisInformationGain !== 0) {
            informationGains[guess] = thisInformationGain;
        }
    }
    for (let guess in informationGains) {
        if (informationGains[guess] > bestInformationGain) {
            bestGuessForInformationGain = guess;
            bestInformationGain = informationGains[guess];
        }
        if (informationGains[guess] > bestInformationGainForWin && this.wordIsCompatible(guess, condition)) {
            bestGuessForWin = guess;
            bestInformationGainForWin = informationGains[guess];
        }
    }
    return [bestGuessForInformationGain, bestGuessForWin, bestInformationGain, bestInformationGainForWin];
};

WordleGame.prototype.won = function () {
    return this.words.length === 1;
};

WordleGame.prototype.applyGuessAndResult = function (guess, result) {
    const newCondition = this.convertPrettyConditionToCondition(guess, result);
    this.applyCondition(newCondition);
};

const prettifyCondition = function (condition) {
    // The string to return. We will add to this using << >>
    let s = '';
    for (let letter in condition.inWord) {
        s += letter + ' is';
        for (let position in condition.inWord[letter]) {
            if (condition.inWord[letter][position]) {
                s += ' in position ' + position;
            } else {
                s += ' not in position ' + position;
            }
            s += ',';
        }
        s += '\n';
    }
    s += 'Not in word: ';
    for (let letter of condition.notInWord) {
        s += letter + ' ';
    }
    return s;
};

const WordleWords = function (solutionsFilename, validGuessesFilename) {
    this.words = [];
    this.validGuesses = [];
    // Read in the solutions file, a json list of words
    fs.readFile(solutionsFilename, 'utf8', (err, data) => {
        if (err) {
            console.log('Error reading solutions file: ' + err);
            return;
        }
        this.words = JSON.parse(data);
    });

  // Read in the valid guesses file
    fs.readFile(validGuessesFilename, 'utf8', (err, data) => {
        if (err) {
            console.log('Error reading valid guesses file: ' + err);
            return;
        }
        this.validGuesses = JSON.parse(data);
    });

  // Add all words to validGuesses
    for (let word of this.words) {
        this.validGuesses.push(word);
    }
    console.log('There are ' + this.words.length + ' solutions and ' + this.validGuesses.length + ' valid guesses.');
    this.baseEntropy = new WordleGame(this.words, this.validGuesses).baseEntropy;
};

WordleWords.prototype.newGame = function () {
    return new WordleGame(this.words, this.validGuesses, this.baseEntropy);
};

const main = function () {
    const wordleWords = new WordleWords('wordlesolver/solutions_nyt.json', 'wordlesolver/nonsolutions_nyt.json');
    const wordle = wordleWords.newGame();

    // Interactive solver
    console.log('Optimal first guess is cached');
    console.log('Enter results as a 5-letter string where each letter is:');
    console.log('  \'b\' for black/gray (not present)');
    console.log('  \'y\' for yellow (present but not at the specified position)');
    console.log('  \'g\' for green (present and at the specified position)');
    console.log('Choose \'1. Best guess for information gain\' if you want the most information, or \'2. Best guess for compatibility\' if you\'re feeling lucky and want a quick win.');
    console.log('For example, a result of yellow, yellow, green, green, gray corresponds to the result string \'yyggb\'');
    console.log('Enter soare');

    // Start with the empty condition
    let condition = new Condition();
    let word = 'soare';
    while (true) {
        let result = '';
        while (true) {
            // Prompt for input and block until we get a valid result
            const result = readline.question('Enter result: ');
            // Validate the result
            let resultOk = true;
            if (result.length !== 5) {
                console.log('Result must be 5 letters long; got ' + result.length);
                resultOk = false;
            }
            for (let i = 0; i < 5; i++) {
                if (result[i] !== 'b' && result[i] !== 'y' && result[i] !== 'g') {
                    console.log('Result must be a combination of the characters \'b\', \'y\', and \'g\'; got ' + result);
                    resultOk = false;
                    break;
                }
            }
            if (resultOk) {
                break;
            }
        }
        const newCondition = wordle.convertPrettyConditionToCondition(word, result);
        condition = wordle.combineConditions(condition, newCondition);
        console.log(word + ' is ' + result);
        console.log('New condition:\n' + prettifyCondition(newCondition));
        console.log('Combined condition:\n' + prettifyCondition(condition));
        wordle.applyCondition(condition);
        const numCompatibleInWordlist = wordle.getCompatibleWords(condition).length;
        const MAX_COMPATIBLE_WORDS = 10;
        if (numCompatibleInWordlist === 0) {
            console.log('No words are compatible with this condition');
            break;
        }
        if (numCompatibleInWordlist === 1) {
            console.log('The word is ' + wordle.words[0]);
            break;
        }
        console.log('There are ' + numCompatibleInWordlist + ' compatible words remaining');
        if (numCompatibleInWordlist <= MAX_COMPATIBLE_WORDS) {
            for (let word of wordle.words) {
                console.log(word + ' ');
            }
        }
        const [bestGuessForInformationGain, bestGuessForWin, infogain1, infogain2] = wordle.findOptimalGuesses(condition);
        if (infogain1 <= infogain2) {
            console.log('The best guess is ' + bestGuessForWin + ' with an information gain of ' + infogain1);
            word = bestGuessForWin;
        } else {
            console.log('1. Best guess for information gain: ' + bestGuessForInformationGain + ' (' + infogain1 + ' bits)');
            console.log('2. Best guess for compatibility: ' + bestGuessForWin + ' (' + infogain2 + ' bits)');
            console.log('Which guess do you want to use? (1 or 2)');
            while (true) {
                const choice = prompt();
                if (choice === '1') {
                    word = bestGuessForInformationGain;
                    break;
                } else if (choice === '2') {
                    word = bestGuessForWin;
                    break;
                } else {
                    console.log('Invalid choice');
                }
            }
        }
    }
};

main();
