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
        var array =  [8, 1, 4, 6, 2, 3, 5, 7]
        print("冒泡排序前: \(array)")
        bubbling(&array)
        let resultArr = bubbling1(array)
        array.bubbling()
        
        print("冒泡排序后: \(array)")
        print("冒泡排序后: \(resultArr)")
        
    }


}

