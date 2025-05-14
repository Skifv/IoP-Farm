/**
 * Функция MyMap - улучшенная версия стандартной функции map() для Arduino
 * Позволяет преобразовывать значения из одного диапазона в другой
 * с поддержкой различных числовых типов и улучшенной точностью
 */

#pragma once

namespace farm::utils
{
    /**
     * Преобразует значение из одного диапазона в другой
     * 
     * @tparam T тип входного значения
     * @tparam U тип выходного значения
     * @param x значение для преобразования
     * @param in_min минимальное значение входного диапазона
     * @param in_max максимальное значение входного диапазона
     * @param out_min минимальное значение выходного диапазона
     * @param out_max максимальное значение выходного диапазона
     * @return преобразованное значение типа U
     */
    template<typename T, typename U>
    U MyMap(T x, T in_min, T in_max, U out_min, U out_max)
    {
        // Проверяем корректность входных диапазонов, чтобы избежать деления на ноль
        if (in_min == in_max) {
            return out_min;
        }
        
        // Используем промежуточный тип double для повышения точности вычислений
          
        double input_range = (double)(in_max - in_min);
        double output_range = (double)(out_max - out_min);

        double delta = (double)(x - in_min);
        
        U result = out_min + (U)(delta * output_range / input_range);
        
        return result;
    }
} 