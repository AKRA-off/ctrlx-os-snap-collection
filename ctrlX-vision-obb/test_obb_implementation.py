#!/usr/bin/env python3
"""
Test Script for YOLOOBB Implementation

This script tests the YOLOOBB class with both 180° and 360° angle modes.
Requires: YOLOv11-OBB ONNX model and test images

Usage:
    python test_obb_implementation.py --model yolov11n-obb.onnx --image test.jpg
"""

import argparse
import cv2
import numpy as np
import os
import sys
from pathlib import Path

# Add project root to path
sys.path.insert(0, str(Path(__file__).parent))

from yoloobb.YOLOOBB import YOLOOBB


def test_basic_inference(model_path, image_path, angle_mode="360"):
    """
    Test basic OBB inference functionality

    Args:
        model_path: Path to ONNX model file
        image_path: Path to test image
        angle_mode: "180" or "360"
    """
    print(f"\n{'='*70}")
    print(f"Test 1: Basic Inference ({angle_mode}° mode)")
    print(f"{'='*70}\n")

    try:
        # Initialize detector
        print(f"Loading model: {model_path}")
        yolo_obb = YOLOOBB(
            model_path,
            conf_thres=0.3,
            iou_thres=0.5,
            angle_mode=angle_mode
        )
        print("✅ Model loaded successfully")

        # Load image
        print(f"Loading image: {image_path}")
        img = cv2.imread(image_path)
        if img is None:
            print(f"❌ Failed to load image: {image_path}")
            return False
        print(f"✅ Image loaded: {img.shape}")

        # Run inference
        print("Running inference...")
        boxes, scores, class_ids, angles = yolo_obb(img)
        print(f"✅ Inference complete: {len(boxes)} objects detected")

        # Print results
        print(f"\n{'─'*70}")
        print("Detection Results:")
        print(f"{'─'*70}")

        if len(boxes) == 0:
            print("⚠️  No objects detected")
        else:
            for i, (box, score, class_id, angle) in enumerate(zip(boxes, scores, class_ids, angles)):
                cx, cy, w, h = box
                angle_deg = np.degrees(angle)
                print(f"Object {i+1}:")
                print(f"  Class ID:    {class_id}")
                print(f"  Confidence:  {score:.3f}")
                print(f"  Center:      ({cx:.1f}, {cy:.1f})")
                print(f"  Size:        {w:.1f} x {h:.1f}")
                print(f"  Angle:       {angle_deg:.1f}° ({angle:.3f} rad)")
                print()

        # Draw detections
        print("Drawing detections...")
        output_img = yolo_obb.draw_obb(img, draw_angle_arrow=True)

        # Save output
        output_path = f"test_output_{angle_mode}.jpg"
        cv2.imwrite(output_path, output_img)
        print(f"✅ Output saved to: {output_path}")

        print(f"\n{'='*70}")
        print(f"Test 1 PASSED ✅")
        print(f"{'='*70}\n")

        return True

    except Exception as e:
        print(f"\n❌ Test 1 FAILED: {str(e)}")
        import traceback
        traceback.print_exc()
        return False


def test_angle_modes_comparison(model_path, image_path):
    """
    Compare results between 180° and 360° modes

    Args:
        model_path: Path to ONNX model file
        image_path: Path to test image
    """
    print(f"\n{'='*70}")
    print(f"Test 2: Angle Mode Comparison (180° vs 360°)")
    print(f"{'='*70}\n")

    try:
        # Load image
        img = cv2.imread(image_path)
        if img is None:
            print(f"❌ Failed to load image: {image_path}")
            return False

        # Test 180° mode
        print("Testing 180° mode...")
        yolo_180 = YOLOOBB(model_path, conf_thres=0.3, iou_thres=0.5, angle_mode="180")
        boxes_180, scores_180, class_ids_180, angles_180 = yolo_180(img)
        print(f"✅ 180° mode: {len(boxes_180)} detections")

        # Test 360° mode
        print("Testing 360° mode...")
        yolo_360 = YOLOOBB(model_path, conf_thres=0.3, iou_thres=0.5, angle_mode="360")
        boxes_360, scores_360, class_ids_360, angles_360 = yolo_360(img)
        print(f"✅ 360° mode: {len(boxes_360)} detections")

        # Compare results
        print(f"\n{'─'*70}")
        print("Angle Comparison:")
        print(f"{'─'*70}")

        min_detections = min(len(angles_180), len(angles_360))
        if min_detections == 0:
            print("⚠️  No detections to compare")
        else:
            print(f"{'Object':<10} {'180° Mode':<15} {'360° Mode':<15} {'Difference':<15}")
            print(f"{'-'*70}")

            for i in range(min_detections):
                angle_180_deg = np.degrees(angles_180[i])
                angle_360_deg = np.degrees(angles_360[i])
                diff = abs(angle_360_deg - angle_180_deg)

                print(f"{i+1:<10} {angle_180_deg:<15.1f} {angle_360_deg:<15.1f} {diff:<15.1f}")

        # Draw comparison
        img_180 = yolo_180.draw_obb(img.copy(), draw_angle_arrow=True)
        img_360 = yolo_360.draw_obb(img.copy(), draw_angle_arrow=True)

        # Create side-by-side comparison
        comparison = np.hstack([img_180, img_360])

        # Add labels
        cv2.putText(comparison, "180° Mode", (50, 50),
                   cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 3)
        cv2.putText(comparison, "360° Mode", (img_180.shape[1] + 50, 50),
                   cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 3)

        output_path = "test_comparison_180_vs_360.jpg"
        cv2.imwrite(output_path, comparison)
        print(f"\n✅ Comparison saved to: {output_path}")

        print(f"\n{'='*70}")
        print(f"Test 2 PASSED ✅")
        print(f"{'='*70}\n")

        return True

    except Exception as e:
        print(f"\n❌ Test 2 FAILED: {str(e)}")
        import traceback
        traceback.print_exc()
        return False


def test_rotated_nms(model_path, image_path):
    """
    Test rotated NMS functionality

    Args:
        model_path: Path to ONNX model file
        image_path: Path to test image with overlapping rotated objects
    """
    print(f"\n{'='*70}")
    print(f"Test 3: Rotated NMS Functionality")
    print(f"{'='*70}\n")

    try:
        # Test with different IoU thresholds
        iou_thresholds = [0.3, 0.5, 0.7]
        img = cv2.imread(image_path)

        if img is None:
            print(f"❌ Failed to load image: {image_path}")
            return False

        print(f"Testing NMS with different IoU thresholds:")
        print(f"{'─'*70}")
        print(f"{'IoU Threshold':<20} {'Detections':<15} {'Change':<15}")
        print(f"{'-'*70}")

        prev_count = None
        for iou_thresh in iou_thresholds:
            yolo_obb = YOLOOBB(model_path, conf_thres=0.3, iou_thres=iou_thresh, angle_mode="360")
            boxes, scores, class_ids, angles = yolo_obb(img)

            change = "" if prev_count is None else f"{len(boxes) - prev_count:+d}"
            print(f"{iou_thresh:<20.1f} {len(boxes):<15} {change:<15}")

            # Save visualization
            output_img = yolo_obb.draw_obb(img.copy(), draw_angle_arrow=True)
            cv2.imwrite(f"test_nms_iou_{iou_thresh:.1f}.jpg", output_img)

            prev_count = len(boxes)

        print(f"\n✅ NMS test outputs saved")

        print(f"\n{'='*70}")
        print(f"Test 3 PASSED ✅")
        print(f"{'='*70}\n")

        return True

    except Exception as e:
        print(f"\n❌ Test 3 FAILED: {str(e)}")
        import traceback
        traceback.print_exc()
        return False


def test_edge_cases(model_path):
    """
    Test edge cases and error handling

    Args:
        model_path: Path to ONNX model file
    """
    print(f"\n{'='*70}")
    print(f"Test 4: Edge Cases and Error Handling")
    print(f"{'='*70}\n")

    tests_passed = 0
    tests_total = 4

    # Test 1: Invalid angle mode
    print("Test 4.1: Invalid angle mode...")
    try:
        yolo_obb = YOLOOBB(model_path, angle_mode="invalid")
        print("❌ Should have raised ValueError")
    except ValueError as e:
        print(f"✅ Correctly raised ValueError: {e}")
        tests_passed += 1

    # Test 2: Empty image
    print("\nTest 4.2: Empty/None image...")
    try:
        yolo_obb = YOLOOBB(model_path, angle_mode="360")
        empty_img = np.zeros((640, 640, 3), dtype=np.uint8)
        boxes, scores, class_ids, angles = yolo_obb(empty_img)
        print(f"✅ Handled empty image: {len(boxes)} detections")
        tests_passed += 1
    except Exception as e:
        print(f"❌ Failed on empty image: {e}")

    # Test 3: Very small confidence threshold
    print("\nTest 4.3: Very low confidence threshold...")
    try:
        yolo_obb = YOLOOBB(model_path, conf_thres=0.01, angle_mode="360")
        print("✅ Initialized with low confidence threshold")
        tests_passed += 1
    except Exception as e:
        print(f"❌ Failed with low threshold: {e}")

    # Test 4: Very high confidence threshold
    print("\nTest 4.4: Very high confidence threshold...")
    try:
        yolo_obb = YOLOOBB(model_path, conf_thres=0.99, angle_mode="360")
        print("✅ Initialized with high confidence threshold")
        tests_passed += 1
    except Exception as e:
        print(f"❌ Failed with high threshold: {e}")

    print(f"\n{'─'*70}")
    print(f"Edge Cases: {tests_passed}/{tests_total} tests passed")
    print(f"{'='*70}")
    print(f"Test 4 {'PASSED ✅' if tests_passed == tests_total else 'PARTIALLY PASSED ⚠️'}")
    print(f"{'='*70}\n")

    return tests_passed == tests_total


def test_performance(model_path, image_path, num_runs=10):
    """
    Test inference performance

    Args:
        model_path: Path to ONNX model file
        image_path: Path to test image
        num_runs: Number of inference runs for timing
    """
    print(f"\n{'='*70}")
    print(f"Test 5: Performance Benchmarking")
    print(f"{'='*70}\n")

    try:
        import time

        # Load model and image
        yolo_obb = YOLOOBB(model_path, conf_thres=0.3, iou_thres=0.5, angle_mode="360")
        img = cv2.imread(image_path)

        if img is None:
            print(f"❌ Failed to load image: {image_path}")
            return False

        print(f"Running {num_runs} inference iterations...")

        times = []
        for i in range(num_runs):
            start = time.perf_counter()
            boxes, scores, class_ids, angles = yolo_obb(img)
            end = time.perf_counter()
            elapsed = (end - start) * 1000  # Convert to ms
            times.append(elapsed)

            if i == 0:
                print(f"  Run {i+1}: {elapsed:.2f}ms ({len(boxes)} detections)")

        # Calculate statistics
        times = np.array(times)
        mean_time = np.mean(times)
        std_time = np.std(times)
        min_time = np.min(times)
        max_time = np.max(times)

        print(f"\n{'─'*70}")
        print("Performance Statistics:")
        print(f"{'─'*70}")
        print(f"Mean inference time:   {mean_time:.2f} ms")
        print(f"Std deviation:         {std_time:.2f} ms")
        print(f"Min time:              {min_time:.2f} ms")
        print(f"Max time:              {max_time:.2f} ms")
        print(f"FPS (average):         {1000/mean_time:.1f}")

        print(f"\n{'='*70}")
        print(f"Test 5 PASSED ✅")
        print(f"{'='*70}\n")

        return True

    except Exception as e:
        print(f"\n❌ Test 5 FAILED: {str(e)}")
        import traceback
        traceback.print_exc()
        return False


def main():
    parser = argparse.ArgumentParser(description='Test YOLOOBB Implementation')
    parser.add_argument('--model', type=str, required=True,
                       help='Path to YOLO-OBB ONNX model file')
    parser.add_argument('--image', type=str, required=True,
                       help='Path to test image')
    parser.add_argument('--tests', type=str, default='all',
                       help='Tests to run: all, basic, comparison, nms, edge, performance')

    args = parser.parse_args()

    # Validate inputs
    if not os.path.exists(args.model):
        print(f"❌ Model file not found: {args.model}")
        sys.exit(1)

    if not os.path.exists(args.image):
        print(f"❌ Image file not found: {args.image}")
        sys.exit(1)

    print(f"\n{'='*70}")
    print(f"YOLOOBB Implementation Test Suite")
    print(f"{'='*70}")
    print(f"Model:  {args.model}")
    print(f"Image:  {args.image}")
    print(f"Tests:  {args.tests}")
    print(f"{'='*70}\n")

    # Run tests
    results = {}

    if args.tests in ['all', 'basic']:
        results['basic_360'] = test_basic_inference(args.model, args.image, angle_mode="360")
        results['basic_180'] = test_basic_inference(args.model, args.image, angle_mode="180")

    if args.tests in ['all', 'comparison']:
        results['comparison'] = test_angle_modes_comparison(args.model, args.image)

    if args.tests in ['all', 'nms']:
        results['nms'] = test_rotated_nms(args.model, args.image)

    if args.tests in ['all', 'edge']:
        results['edge'] = test_edge_cases(args.model)

    if args.tests in ['all', 'performance']:
        results['performance'] = test_performance(args.model, args.image)

    # Print summary
    print(f"\n{'='*70}")
    print(f"TEST SUMMARY")
    print(f"{'='*70}")

    passed = sum(1 for v in results.values() if v)
    total = len(results)

    for test_name, result in results.items():
        status = "✅ PASSED" if result else "❌ FAILED"
        print(f"{test_name:<30} {status}")

    print(f"{'='*70}")
    print(f"Total: {passed}/{total} tests passed")
    print(f"{'='*70}\n")

    if passed == total:
        print("🎉 All tests passed! YOLOOBB implementation is working correctly.\n")
        sys.exit(0)
    else:
        print("⚠️  Some tests failed. Please review the errors above.\n")
        sys.exit(1)


if __name__ == '__main__':
    main()
