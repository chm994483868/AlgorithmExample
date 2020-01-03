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
        var array = [8, 1, 4, 6, 2, 3, 5, 7, 1]
        print("快速排序前的序列 \(array)")
        quickSort1(&array, 0, array.count - 1)
        print("快速排序后的序列 \(array)")
    }
}

