window.addEventListener('load', onLoadWindow);

function onLoadWindow(event) {
    fetch('/jsonRequest',) //Запрос данных полей ввода из ардуино
        .then(function (response) {
            response.json().then(function (settings) {
                writeToHtmlUserValue(settings);
            })
        })
}

//Функция возвращает объект с данными всех input, select, checbox со страницы
function readfromHtmlUserValue() {
    const elements = document.querySelectorAll("input, select");
    const settings = {};
    elements.forEach(element => {
        const key = element.name || element.id || 'unnamed';
        if (element.type === "checkbox") {
            settings[key] = element.checked;
        } else if (element.type === "number") {
            settings[key] = element.valueAsNumber;
        } else if (element.tagName === "SELECT") {
            settings[key] = parseInt(element.value.trim());
        } else {
            settings[key] = element.value.trim();
        }
    });
    return settings;
}

//Функция заполняет данными из принятого объека все input, select, checbox на странице
function writeToHtmlUserValue(settings) {
    const e = document.querySelectorAll("input, select");
    e.forEach(e => {
        const key = e.name || e.id;
        if (!key || !(key in settings)) return;

        const value = settings[key];

        if (e.type === "checkbox") {
            e.checked = Boolean(value);
        } else if (e.type === "number") {
            e.valueAsNumber = Number(value);
        } else if (e.tagName === "SELECT") {
            e.value = String(value);
        } else {
            e.value = value;
        }
    });
}

function saveSettings() {
    let Settings = readfromHtmlUserValue();
    console.log(JSON.stringify(Settings, null, 2));
    // if (confirm("Зберегти наступні налаштування? " + JSON.stringify(Settings, null, 2))) {
        document.getElementById("form").style.display = "none"; //Скрываем блок полей ввода
        document.getElementById("formend").style.display = "block";//Показываем блок результата отправки данных

        fetch('/jsonPOST', {
            method: 'POST',
            body: JSON.stringify(Settings)
        }
        ).then(//Отправка данных
            successResponse => {
                if (successResponse.status == 200) {//Проверка получения данных
                    document.getElementById("endmsg").textContent = "Настройки сохранились!";
                } else {
                    document.getElementById("endmsg").textContent = "Не удалось сохранить!";
                }
            },
            failResponse => {
                document.getElementById("endmsg").textContent = "Не удалось сохранить!";
            }
        );
    // }
};