/*
* Gpio.h
*
* Created by Tilong on 2019/12/21.
*/

#ifndef NATIVEGPIO_GPIO_H
#define NATIVEGPIO_GPIO_H

static jboolean gpio_export(JNIEnv *env, jclass thiz, jint pin_num);

static jboolean gpio_unexport(JNIEnv *env, jclass thiz, jint pin_num);

static jboolean setDirection(JNIEnv *env, jclass thiz, jint pin_num, jint direction);

static jint getDirection(JNIEnv *env, jclass clazz, jint pin_num);

static jboolean setValue(JNIEnv *env, jclass clazz, jint pin_num, jboolean value);

static jboolean getValue(JNIEnv *env, jclass clazz, jint pin_num);

#endif //NATIVEGPIO_GPIO_H
