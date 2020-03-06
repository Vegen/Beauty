package com.vegen.beauty

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*

/**
 * Created by vegen on 2020/3/5.
 * Description: 美颜后的页面
 */
class BeautyActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        var originBitmap = BitmapFactory.decodeResource(resources, R.drawable.testPic)
        val startTime = System.currentTimeMillis()
        beauty(originBitmap)
        val endTime = System.currentTimeMillis()
        btn_beauty.text = "美颜完啦，用时：${(endTime - startTime) * 1.0f / 1000f}s"
        iv_origin.setImageBitmap(originBitmap)
    }

    /**
     * 美化
     */
    external fun beauty(bitmap: Bitmap): Int
}