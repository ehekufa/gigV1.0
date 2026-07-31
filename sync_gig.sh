#!/bin/bash

# 1. Удаляем старые сгенерированные файлы для чистоты
rm -f *.mylang

# 2. Копируем все файлы .gig в формат .mylang для GitHub
for file in *.gig; do
    if [ -f "$file" ]; then
        # Берём имя файла без расширения
        name="${file%.gig}"
        
        # Добавляем в начало комментарий-метку для людей
        echo "# Язык: gig (Маскировка под Clean для зелёного цвета)" > "${name}.mylang"
        cat "$file" >> "${name}.mylang"
        
        # Добавляем сгенерированный файл в индекс Git
        git add "${name}.mylang"
    fi
done

echo "✅ Файлы gig успешно синхронизированы и замаскированы под Clean!"
