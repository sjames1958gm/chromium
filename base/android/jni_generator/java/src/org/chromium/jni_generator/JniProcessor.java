// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.jni_generator;

import com.google.auto.service.AutoService;
import com.google.common.base.Charsets;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import com.squareup.javapoet.AnnotationSpec;
import com.squareup.javapoet.ArrayTypeName;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.FieldSpec;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterSpec;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;

import org.chromium.base.annotations.JniStaticNatives;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Generated;
import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.Processor;
import javax.annotation.processing.RoundEnvironment;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.type.ArrayType;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;
import javax.tools.Diagnostic;

/**
 * Annotation processor that finds inner interfaces annotated with
 * @JniStaticNatives and generates a class with native bindings
 * (GEN_JNI) and a class specific wrapper class with name (classnameJni)
 *
 * NativeClass - refers to the class that contains all native declarations.
 * NativeWrapperClass - refers to the class that is generated for each class
 * containing an interface annotated with JniStaticNatives.
 *
 */
@AutoService(Processor.class)
public class JniProcessor extends AbstractProcessor {
    private static final Class<JniStaticNatives> JNI_STATIC_NATIVES_CLASS = JniStaticNatives.class;

    static final String NATIVE_WRAPPER_CLASS_POSTFIX = "Jni";

    static final String NATIVE_CLASS_NAME_STR = "GEN_JNI";
    static final String NATIVE_CLASS_PACKAGE_NAME = "org.chromium.base.natives";
    static final ClassName NATIVE_CLASS_NAME =
            ClassName.get(NATIVE_CLASS_PACKAGE_NAME, NATIVE_CLASS_NAME_STR);
    static final String NATIVE_TEST_FIELD_NAME = "TESTING_ENABLED";
    static final boolean TESTING_ENABLED = false;

    // Builder for NativeClass which will hold all our native method declarations.
    private TypeSpec.Builder mNativesBuilder;

    // Hash function for native method names.
    private static MessageDigest sNativeMethodHashFunction;

    // If true, native methods in GEN_JNI will be named as a hash of their descriptor.
    private static final boolean USE_HASH_FOR_METHODS = !ProcessorArgs.IS_JAVA_DEBUG;

    // Limits the number characters of the Base64 encoded hash
    // of the method descriptor used as name of the generated
    // native method in GEN_JNI (prefixed with "M")
    private static final int MAX_CHARS_FOR_HASHED_NATIVE_METHODS = 8;

    // Types that are non-primitives and should not be
    // casted to objects in native method declarations.
    static final ImmutableSet JNI_OBJECT_TYPE_EXCEPTIONS =
            ImmutableSet.of("java.lang.String", "java.lang.Throwable", "java.lang.Class", "void");

    static String getNameOfWrapperClass(String containingClassName) {
        return containingClassName + NATIVE_WRAPPER_CLASS_POSTFIX;
    }

    @Override
    public Set<String> getSupportedAnnotationTypes() {
        return ImmutableSet.of(JNI_STATIC_NATIVES_CLASS.getCanonicalName());
    }

    @Override
    public SourceVersion getSupportedSourceVersion() {
        return SourceVersion.latestSupported();
    }

    public JniProcessor() {
        FieldSpec.Builder testingFlagBuilder =
                FieldSpec.builder(TypeName.BOOLEAN, NATIVE_TEST_FIELD_NAME)
                        .addModifiers(Modifier.STATIC, Modifier.PUBLIC);
        if (TESTING_ENABLED) {
            testingFlagBuilder.initializer("true");
        }

        // State of mNativesBuilder needs to be preserved between processing rounds.
        mNativesBuilder = TypeSpec.classBuilder(NATIVE_CLASS_NAME)
                                  .addAnnotation(createGeneratedAnnotation())
                                  .addModifiers(Modifier.PUBLIC, Modifier.FINAL)
                                  .addField(testingFlagBuilder.build());

        try {
            sNativeMethodHashFunction = MessageDigest.getInstance("MD5");
        } catch (NoSuchAlgorithmException e) {
            // MD5 support is required for all Java platforms so this should never happen.
            printError(e.getMessage());
        }
    }

    /**
     * Processes annotations that match getSupportedAnnotationTypes()
     * Called each 'round' of annotation processing, must fail gracefully if set is empty.
     */
    @Override
    public boolean process(
            Set<? extends TypeElement> annotations, RoundEnvironment roundEnvironment) {
        // Do nothing on an empty round.
        if (annotations.isEmpty()) {
            return true;
        }

        List<JavaFile> writeQueue = Lists.newArrayList();
        for (Element e : roundEnvironment.getElementsAnnotatedWith(JNI_STATIC_NATIVES_CLASS)) {
            // @JniStaticNatives can only annotate types so this is safe.
            TypeElement type = (TypeElement) e;

            if (!e.getKind().isInterface()) {
                printError("@JniStaticNatives must annotate an interface", e);
            }

            // Interface must be nested within a class.
            Element outerElement = e.getEnclosingElement();
            if (!(outerElement instanceof TypeElement)) {
                printError(
                        "Interface annotated with @JNIInterface must be nested within a class", e);
            }

            TypeElement outerType = (TypeElement) outerElement;
            ClassName outerTypeName = ClassName.get(outerType);
            String outerClassName = outerTypeName.simpleName();
            String packageName = outerTypeName.packageName();

            // Get all methods in annotated interface.
            List<ExecutableElement> interfaceMethods = getMethodsFromType(type);

            // Map from the current method names to the method spec for a static native
            // method that will be in our big NativeClass.
            // Collisions are not allowed - no overloading.
            Map<String, MethodSpec> methodMap =
                    createNativeMethodSpecs(interfaceMethods, outerTypeName);

            // Add these to our NativeClass.
            for (MethodSpec method : methodMap.values()) {
                mNativesBuilder.addMethod(method);
            }

            // Generate a NativeWrapperClass for outerType by implementing the
            // annotated interface. Need to pass it the method map because each
            // method overridden will be a wrapper that calls its
            // native counterpart in NativeClass.
            boolean isNativesInterfacePublic = type.getModifiers().contains(Modifier.PUBLIC);
            TypeSpec nativeWrapperClassSpec =
                    createNativeWrapperClassSpec(getNameOfWrapperClass(outerClassName),
                            isNativesInterfacePublic, type, methodMap);

            // Queue this file for writing.
            // Can't write right now because the wrapper class depends on NativeClass
            // to be written and we can't write NativeClass until all @JNINatives
            // interfaces are processed because each will add new native methods.
            JavaFile file = JavaFile.builder(packageName, nativeWrapperClassSpec).build();
            writeQueue.add(file);
        }

        // Nothing needs to be written this round.
        if (writeQueue.size() == 0) {
            return true;
        }

        try {
            // Need to write NativeClass first because the wrapper classes
            // depend on it.
            JavaFile nativeClassFile =
                    JavaFile.builder(NATIVE_CLASS_PACKAGE_NAME, mNativesBuilder.build()).build();

            nativeClassFile.writeTo(processingEnv.getFiler());

            for (JavaFile f : writeQueue) {
                f.writeTo(processingEnv.getFiler());
            }
        } catch (Exception e) {
            processingEnv.getMessager().printMessage(Diagnostic.Kind.ERROR, e.getMessage());
        }
        return true;
    }

    List<ExecutableElement> getMethodsFromType(TypeElement t) {
        List<ExecutableElement> methods = Lists.newArrayList();
        for (Element e : t.getEnclosedElements()) {
            if (e.getKind() == ElementKind.METHOD) {
                methods.add((ExecutableElement) e);
            }
        }
        return methods;
    }

    /**
     * Gets method name for methods inside of NativeClass
     */
    String getNativeMethodName(String packageName, String className, String oldMethodName) {
        // e.g. org.chromium.base.Foo_Class.bar
        // => org_chromium_base_Foo_1Class_bar()
        String descriptor = String.format("%s.%s.%s", packageName, className, oldMethodName)
                                    .replaceAll("_", "_1")
                                    .replaceAll("\\.", "_");
        if (USE_HASH_FOR_METHODS) {
            // Must start with a character.
            byte[] hash = sNativeMethodHashFunction.digest(descriptor.getBytes(Charsets.UTF_8));

            String methodName = "M"
                    + Base64.getEncoder()
                              .encodeToString(hash)
                              .replace("/", "_")
                              .replace("+", "$")
                              .replace("=", "");

            return methodName.substring(
                    0, Math.min(MAX_CHARS_FOR_HASHED_NATIVE_METHODS, methodName.length()));
        }
        return descriptor;
    }

    /**
     * Creates method specs for the native methods of NativeClass given
     * the method declarations from a JNINative annotated interface
     * @param interfaceMethods method declarations from a JNINative annotated interface
     * @param outerType ClassName of class that contains the annotated interface
     * @return map from old method name to new native method specification
     */
    Map<String, MethodSpec> createNativeMethodSpecs(
            List<ExecutableElement> interfaceMethods, ClassName outerType) {
        Map<String, MethodSpec> methodMap = Maps.newTreeMap();
        for (ExecutableElement m : interfaceMethods) {
            String oldMethodName = m.getSimpleName().toString();
            String newMethodName = getNativeMethodName(
                    outerType.packageName(), outerType.simpleName(), oldMethodName);
            MethodSpec.Builder builder = MethodSpec.methodBuilder(newMethodName)
                                                 .addModifiers(Modifier.PUBLIC)
                                                 .addModifiers(Modifier.FINAL)
                                                 .addModifiers(Modifier.STATIC)
                                                 .addModifiers(Modifier.NATIVE);
            builder.addJavadoc(createNativeMethodJavadocString(outerType, m));

            copyMethodParamsAndReturnType(builder, m, true);
            if (methodMap.containsKey(oldMethodName)) {
                printError("Overloading is not currently implemented with this processor ", m);
            }
            methodMap.put(oldMethodName, builder.build());
        }
        return methodMap;
    }

    /**
     * Creates a generated annotation that contains the name of this class.
     */
    static AnnotationSpec createGeneratedAnnotation() {
        return AnnotationSpec.builder(Generated.class)
                .addMember("value", String.format("\"%s\"", JniProcessor.class.getCanonicalName()))
                .build();
    }

    void printError(String s) {
        processingEnv.getMessager().printMessage(Diagnostic.Kind.ERROR, s);
    }

    void printError(String s, Element e) {
        processingEnv.getMessager().printMessage(Diagnostic.Kind.ERROR,
                String.format("Error processing @JniStaticNatives interface: %s", s), e);
    }

    /**
     * Creates a class spec for an implementation of an @JNINatives annotated interface that will
     * wrap calls to the NativesClass which contains the actual native method declarations.
     *
     * @param name name of the wrapper class.
     * @param isPublic if true, a public modifier will be added to this native wrapper.
     * @param nativeInterface the @JNINatives annotated type that this native wrapper
     *                        will implement.
     * @param methodMap a map from the old method name to the new method spec in NativeClass.
     * */
    TypeSpec createNativeWrapperClassSpec(String name, boolean isPublic,
            TypeElement nativeInterface, Map<String, MethodSpec> methodMap) {
        TypeName nativeInterfaceType = TypeName.get(nativeInterface.asType());

        TypeSpec.Builder builder = TypeSpec.classBuilder(name)
                                           .addModifiers(Modifier.FINAL)
                                           .addAnnotation(createGeneratedAnnotation())
                                           .addSuperinterface(nativeInterfaceType);
        if (isPublic) {
            builder.addModifiers(Modifier.PUBLIC);
        }

        // Holds the test natives target if it is set.
        FieldSpec testTarget = FieldSpec.builder(nativeInterfaceType, "testInst")
                                       .addModifiers(Modifier.STATIC)
                                       .build();

        ParameterSpec testNativesParam =
                ParameterSpec.builder(nativeInterfaceType, "testNatives").build();

        // Target is a field that holds an instance of some NativeInterface.
        // Is initialized with an instance of this class.
        FieldSpec target = FieldSpec.builder(nativeInterfaceType, "mNatives")
                                   .addModifiers(Modifier.PRIVATE, Modifier.STATIC)
                                   .addModifiers(Modifier.FINAL)
                                   .initializer("new $N()", name)
                                   .build();

        builder.addField(target);
        builder.addField(testTarget);

        // Getter for target or testing instance if flag in GEN_JNI is set.
        MethodSpec instanceGetter =
                MethodSpec.methodBuilder("get")
                        .addModifiers(Modifier.PUBLIC, Modifier.STATIC)
                        .returns(nativeInterfaceType)
                        .beginControlFlow("if ($T.$N)", NATIVE_CLASS_NAME, NATIVE_TEST_FIELD_NAME)
                        .addStatement("return $N", testTarget)
                        .endControlFlow()
                        .addStatement("return $N", target)
                        .build();

        // Sets testTarget to passed in Natives instance.
        MethodSpec setInstanceForTesting =
                MethodSpec.methodBuilder("setForTesting")
                        .returns(TypeName.VOID)
                        .addParameter(testNativesParam)
                        .addModifiers(Modifier.PUBLIC, Modifier.STATIC, Modifier.FINAL)
                        .addStatement("$N = $N", testTarget, testNativesParam)
                        .build();

        builder.addMethod(instanceGetter);
        builder.addMethod(setInstanceForTesting);

        for (Element enclosed : nativeInterface.getEnclosedElements()) {
            if (enclosed.getKind() != ElementKind.METHOD) {
                printError(
                        "Cannot have a non-method in a @JNINatives annotated interface", enclosed);
            }

            // ElementKind.Method is ExecutableElement so this cast is safe.
            // interfaceMethod will is the method we are overloading.
            ExecutableElement interfaceMethod = (ExecutableElement) enclosed;

            // Method in NativesClass that we'll be calling.
            MethodSpec nativesMethod = methodMap.get(interfaceMethod.getSimpleName().toString());

            // Add a matching method in this class that overrides the declaration
            // in nativeInterface. It will just call the actual natives method in
            // NativeClass.
            builder.addMethod(createNativeWrapperMethod(interfaceMethod, nativesMethod));
        }

        return builder.build();
    }

    /**
     * Creates a wrapper method that overrides interfaceMethod and calls staticNativeMethod.
     * @param interfaceMethod method that will be overridden in a @JNINatives annotated interface.
     * @param staticNativeMethod method that will be called in NativeClass.
     */
    MethodSpec createNativeWrapperMethod(
            ExecutableElement interfaceMethod, MethodSpec staticNativeMethod) {
        // Method will have the same name and be public.
        MethodSpec.Builder builder =
                MethodSpec.methodBuilder(interfaceMethod.getSimpleName().toString())
                        .addModifiers(Modifier.PUBLIC)
                        .addAnnotation(Override.class);

        // Method will need the same params and return type as the one we're overriding.
        copyMethodParamsAndReturnType(builder, interfaceMethod);

        // Add return if method return type is not void.
        if (!interfaceMethod.getReturnType().toString().equals("void")) {
            // Also need to cast because non-primitives are Objects in NativeClass.
            builder.addCode("return ($T)", interfaceMethod.getReturnType());
        }

        // Make call to native function.
        builder.addCode("$T.$N(", NATIVE_CLASS_NAME, staticNativeMethod);

        // Add params to native call.
        ArrayList<String> paramNames = new ArrayList<>();
        for (VariableElement param : interfaceMethod.getParameters()) {
            paramNames.add(param.getSimpleName().toString());
        }

        builder.addCode(String.join(", ", paramNames) + ");\n");
        return builder.build();
    }

    void copyMethodParamsAndReturnType(MethodSpec.Builder builder, ExecutableElement method) {
        copyMethodParamsAndReturnType(builder, method, false);
    }

    boolean shouldDowncastToObjectForJni(TypeName t) {
        if (t.isPrimitive()) {
            return false;
        }
        // There are some non-primitives that should not be downcasted.
        return !JNI_OBJECT_TYPE_EXCEPTIONS.contains(t.toString());
    }

    TypeName toTypeName(TypeMirror t, boolean useJni) {
        if (t.getKind() == TypeKind.ARRAY) {
            return ArrayTypeName.of(toTypeName(((ArrayType) t).getComponentType(), useJni));
        }
        TypeName typeName = TypeName.get(t);
        if (useJni && shouldDowncastToObjectForJni(typeName)) {
            return TypeName.OBJECT;
        }
        return typeName;
    }

    /**
     * Since some types may decay to objects in the native method
     * this method returns a javadoc string that contains the
     * type information from the old method. The fully qualified
     * descriptor of the method is also included since the name
     * may be hashed.
     */
    String createNativeMethodJavadocString(ClassName outerType, ExecutableElement oldMethod) {
        ArrayList<String> docLines = new ArrayList<>();

        // Class descriptor.
        String descriptor = String.format("%s.%s.%s", outerType.packageName(),
                outerType.simpleName(), oldMethod.getSimpleName().toString());
        docLines.add(descriptor);

        // Parameters.
        for (VariableElement param : oldMethod.getParameters()) {
            TypeName paramType = TypeName.get(param.asType());
            String paramTypeName = paramType.toString();
            String name = param.getSimpleName().toString();
            docLines.add(String.format("@param %s (%s)", name, paramTypeName));
        }

        // Return type.
        docLines.add(String.format("@return (%s)", oldMethod.getReturnType().toString()));

        return String.join("\n", docLines) + "\n";
    }

    void copyMethodParamsAndReturnType(
            MethodSpec.Builder builder, ExecutableElement method, boolean useJniTypes) {
        for (VariableElement param : method.getParameters()) {
            builder.addParameter(createParamSpec(param, useJniTypes));
        }
        TypeMirror givenReturnType = method.getReturnType();
        TypeName returnType = toTypeName(givenReturnType, useJniTypes);

        builder.returns(returnType);
    }

    ParameterSpec createParamSpec(VariableElement param, boolean useJniObjects) {
        TypeName paramType = toTypeName(param.asType(), useJniObjects);
        return ParameterSpec.builder(paramType, param.getSimpleName().toString())
                .addModifiers(param.getModifiers())
                .build();
    }
}
