// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MEOCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MEO_PROJECTV2_API AMEOCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMEOCameraPawn();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//	카메라가 추종할 타겟 캐릭터 설정 (리스폰 시 PlayerController가 호출) (캐릭터가 죽어도 카메라도 같이 죽지 않음)
	UFUNCTION(BlueprintCallable, Category = "MEO|Camera")
	void SetTargetCharacter(AActor* NewTargetCharacter);

	//	마우스 휠 입력을 받아 카메라 줌(Zoom In/Out) 목표치를 조정
	UFUNCTION(BlueprintCallable, Category = "MEO|Camera")
	void AdjustZoom(float ZoomDelta);

	//	타겟 캐릭터가 사망했을 때 호출되어 시점을 고정하거나 관전 모드로 전환
	UFUNCTION(BlueprintCallable, Category = "MEO|Camera")
	void OnTargetCharacterDied();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FollowCamera;

	//	카메라가 타겟을 따라가는 보간(Interpolation) 속도. 속도가 낮을수록 묵직하게 따라감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
	float TargetFollowSpeed = 10.0f;

	//	카메라 줌 인/아웃의 최소 및 최대 거리 한계치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
	float MinArmLength = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
	float MaxArmLength = 2200.0f;

private:
	//	현재 카메라가 쫒고있는 대상 액터 포인트
	UPROPERTY()
	AActor* TargetActor = nullptr;

	//	마우스 휠로 설정된 목표 줌 거리 (Tick에서 FInterpTo를 통해 이 수치로 서서히 이동)
	float DesiredArmLength = 1500.0f;

	//	매 프레임 타겟 위치로 카메라를 부드럽게 이동시키는 내부 함수
	void UpdateCameraPosition(float DeltaSeconds);

};
