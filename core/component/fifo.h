/*
 * @Author: 苗金标
 * @Date: 2023-04-03 18:38:13
 * @LastEditors: 苗金标
 * @LastEditTime: 2023-06-08 14:20:33
 * @Description: 
 */

#ifndef COM_FIFO_H
#define COM_FIFO_H
#include "../common.h"
#include "../config.h"

    namespace component{
        template <typename T>
        class fifo {
            protected:
                T *buffer;
                uint32_t size;
                uint32_t wptr;
                bool wstage;
                uint32_t rptr;
                bool rstage;
            public:
                fifo(uint32_t size);
                ~fifo();
                void flush();
                bool push(T element);
                bool pop(T *element);
                bool get_front(T *element);
                bool get_tail(T *element);
                uint32_t get_size();
                bool is_empty();
                bool is_full();
        };
        template <typename T>
        fifo<T>::fifo(uint32_t size){
            buffer = new T[size];
            this->size = size;
            wptr = 0;
            wstage = false;
            rptr = 0;
            rstage = false;
        }

        template <typename T>
        fifo<T>::~fifo(){
            delete[] buffer;
        }

        template <typename T>
        void fifo<T>::flush(){
            wptr = 0;
            wstage = false;
            rptr = 0;
            rstage = false;
        }

        template <typename T>
        bool fifo<T>::push(T element){
            if(is_full()){
                return false;
            }
            buffer[wptr++] = element;
            if(wptr >= size){
                wptr = 0;
                wstage = !wstage;
            }
            return true;
        }
    
        template<typename T>
        bool fifo<T>::pop(T *element)
        {
            if(is_empty())
            {
                return false;
            }

            *element = buffer[rptr++];

            if(rptr >= size)
            {
                rptr = 0;
                rstage = !rstage;
            }

            return true;
        }

        template<typename T>
        bool fifo<T>::get_front(T *element){
            if(this->is_empty())
                return false;
            *element = this->buffer[rptr];
            return true;
        }

        template<typename T>
        bool fifo<T>::get_tail(T *element){
            if(this->is_empty())
                return false;
            *element = this->buffer[(wptr + this->size - 1) % this->size];
            return true;
        }

        template<typename T>
        uint32_t fifo<T>::get_size(){
            return this->is_full() ? this->size : ((this->wptr + this->size - this->rptr) % this->size);
        }
    
        template<typename T>
        bool fifo<T>::is_empty()
        {
            return (rptr == wptr) && (rstage == wstage);
        }

        template<typename T>
        bool fifo<T>::is_full()
        {
            return (rptr == wptr) && (rstage != wstage);
        }
    }
#endif