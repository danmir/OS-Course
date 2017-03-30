port=3333
declare -a field

function handshake {
    # Выясняем роли
    echo "Ждем ответа второго игрока..."
    reply=$(nc localhost $port)

    if [[ $reply ]] ; then
        amIFirst=false
        me='O'
    else
        amIFirst=true
        me='X'
        until echo "waiting" | nc -l $port 2>/dev/null; do :; done
    fi
    currentPlayer='X'
    clear
}

function initField {
    num_rows=3
    num_columns=3
    emptyChar='-'
    gameResult=
    for (( i = 0; i < $num_rows; i++ )); do
        for (( j = 0; j < $num_columns; j++ )); do
            let "index = $i * $num_rows + $j"
            field[$index]=$emptyChar
        done
    done
}

function printField {
    tput sc
    local startColumnPos=$1
    local startRowPos=$2

    local row=0
    local index
    while [ "$row" -lt "$num_rows" ]; do
        local column=0
        while [ "$column" -lt "$num_columns" ]; do
            let "index = $row * $num_rows + $column"
            tput cup `expr $startColumnPos + $row` `expr $startRowPos + $column`
            echo "${field[index]} \c"
            let "column += 1"
        done
        echo
        let "row += 1"
    done
    tput rc
}

function readInput {
    while true; do
        read -p "Введите строку и столбец для хода [1-3] [1-3]: " urow ucol
        let "urow -= 1"
        let "ucol -= 1"
        if ! [[ $urow -ge 0 && $urow -le 2 && $ucol -ge 0 && $ucol -le 2 ]]; then
            echo 'Неверные данные. Повторите ввод'
            continue
        fi
        local index
        let "index = $urow * $num_rows + $ucol"
        if [[ ${field[$index]} = 'X' || ${field[$index]} = 'O' ]]; then
            echo 'Клетка уже занята. Повторите ввод'
            continue
        fi
        break
    done
}

function runRound {
    if [[ $me = $currentPlayer ]]; then
        echo "Мой ход - " $me
        readInput
        local index
        let "index = $urow * $num_rows + $ucol"
        field[$index]=$me
        echo "${field[*]}" | nc -l $port 2>/dev/null
    else
        echo "Сейчас ходит другой игрок"
        while true; do
            newField=$(nc localhost $port)
            if [[ $newField ]]; then
                break
            fi
        done
        field=($newField)
    fi

    # Меняем текущего игрока
    if [[ $currentPlayer = 'X' ]]; then
        currentPlayer='O'
    else
        currentPlayer='X'
    fi
}

function checkState {
    # Проверяем ничью
    local isDraw=true
    for (( i = 0; i < $num_rows; i++ )); do
        for (( j = 0; j < $num_columns; j++ )); do
            if [[ $(getFieldCell i j) = $emptyChar ]]; then
                isDraw=
                break
            fi
        done
    done

    if [[ $isDraw ]]; then
        gameResult="Draw"
    fi

    # Проверяем строки и столбцы
    for (( i = 0; i < $num_rows; i++ )); do
        if [[ $(getFieldCell $i 0) != $emptyChar &&\
              $(getFieldCell $i 0) = $(getFieldCell $i 1) &&\
              $(getFieldCell $i 1) = $(getFieldCell $i 2) ]]; then
                  gameResult=$(getFieldCell $i 0)
        fi
    done
    for (( i = 0; i < $num_columns; i++ )); do
        if [[ $(getFieldCell 0 $i) != $emptyChar &&\
              $(getFieldCell 0 $i) = $(getFieldCell 1 $i) &&\
              $(getFieldCell 1 $i) = $(getFieldCell 2 $i) ]]; then
                  gameResult=$(getFieldCell 0 $i)
        fi
    done

    # Проверяем диагонали
    if [[ $(getFieldCell 0 0) != $emptyChar &&\
          $(getFieldCell 0 0) = $(getFieldCell 1 1) &&\
          $(getFieldCell 1 1) = $(getFieldCell 2 2) ]]; then
              gameResult=$(getFieldCell 0 0)
    elif [[ $(getFieldCell 0 2) != $emptyChar &&\
            $(getFieldCell 0 2) = $(getFieldCell 1 1) &&\
            $(getFieldCell 1 1) = $(getFieldCell 2 0) ]]; then
                gameResult=$(getFieldCell 0 2)
    fi
}

function getFieldCell {
    local index
    let "index = $1 * $num_rows + $2"
    echo ${field[$index]}
}

function printResults {
    if [[ $gameResult = 'X' ]]; then
        message="X Выиграли"
    elif [[ $gameResult = 'O' ]]; then
        message="O Выиграли"
    elif [[ $gameResult = "Draw" ]]; then
        message="Ничья"
    fi

    tput sc
    tput cup `expr $1 - 2` $2
    echo $message
    tput rc

    read -p 'Нажмите любую клавишу для выхода'
    clear
    exit
}

function getCenter {
    cols=$(tput cols)
    rows=$(tput lines)
    centercol=$((cols/2))
    centerrow=$((rows/2))
}

clear
handshake
initField
getCenter
printField $centerrow $centercol
while true; do
    runRound
    clear
    getCenter
    printField $centerrow $centercol
    checkState

    if [[ $gameResult ]]; then
        printResults $centerrow $centercol
    fi
done
