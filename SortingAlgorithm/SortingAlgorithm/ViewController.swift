//
//  ViewController.swift
//  SortingAlgorithm
//
//  Created by HM C on 2019/12/25.
//  Copyright © 2019 HM C. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        
        let array =  [8, 1, 4, 6, 2, 3, 5, 7, 1]
        
        print("希尔排序前: \(array)")
//        array.shellSort()
//        shellSort(&array)
        let array1 = shellSort1(array)
        print("希尔排序后: \(array)")
        print("希尔排序后: \(array1)")
    }
}

